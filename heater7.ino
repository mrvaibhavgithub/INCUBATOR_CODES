#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <avr/wdt.h>   

#define DHTTYPE DHT22

// DHT Sensors
DHT dht1(2, DHTTYPE);
DHT dht2(3, DHTTYPE);
DHT dht3(4, DHTTYPE);
DHT dht4(5, DHTTYPE);

// Relays
#define RELAY_HEATER1 6
#define RELAY_HEATER2 7
#define RELAY_FOGGER  8
#define RELAY_FAN     9

LiquidCrystal_I2C lcd(0x27, 16, 2);

float targetTemp = 37.5;
float targetHumidity = 55.0;
float hysteresis = 0.3;

unsigned long lastRead = 0;
const unsigned long interval = 2000;     // Read interval
const unsigned long freezeTimeout = 120000; // 60 sec sensor freeze timeout

float lastTempGroup1 = 0;
float lastTempGroup2 = 0;
unsigned long lastChangeTime = 0;

// Auto reboot variables
unsigned long rebootInterval = 20UL * 60UL * 1000UL; // 20 min in ms
unsigned long startTime = 0;

// LCD freeze detection
float lastDisplayT1 = -100;
float lastDisplayT2 = -100;
float lastDisplayH  = -100;
unsigned long lastDisplayChange = 0;
const unsigned long displayFreezeTimeout = 180000; // 3 minutes

void setup() {
  dht1.begin(); dht2.begin(); dht3.begin(); dht4.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Arduino Booting");
  delay(2000);
  lcd.clear();

  pinMode(RELAY_HEATER1, OUTPUT);
  pinMode(RELAY_HEATER2, OUTPUT);
  pinMode(RELAY_FOGGER, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);

  digitalWrite(RELAY_HEATER1, LOW);
  digitalWrite(RELAY_HEATER2, LOW);
  digitalWrite(RELAY_FOGGER, LOW);
  digitalWrite(RELAY_FAN, HIGH);

  startTime = millis();
  lastDisplayChange = millis();
}

void loop() {
  // 🔹 20-minute auto reboot
  if (millis() - startTime >= rebootInterval) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rebooting...");
    delay(500);
    wdt_enable(WDTO_15MS);
    while (1);
  }

  if (millis() - lastRead >= interval) {
    lastRead = millis();

    float t1 = dht1.readTemperature();
    float t2 = dht2.readTemperature();
    float t3 = dht3.readTemperature();
    float t4 = dht4.readTemperature();

    float h1 = dht1.readHumidity();
    float h2 = dht2.readHumidity();
    float h3 = dht3.readHumidity();
    float h4 = dht4.readHumidity();

    float tempGroup1 = average(t1, t2);
    float tempGroup2 = average(t3, t4);
    float avgHumidity = average(h1, h2, h3, h4);

    // 🔹 Track temperature change for freeze detection
    if (tempGroup1 != lastTempGroup1 || tempGroup2 != lastTempGroup2) {
      lastChangeTime = millis();
      lastTempGroup1 = tempGroup1;
      lastTempGroup2 = tempGroup2;
    }

    // 🔹 Sensor Freeze → Immediate Reboot
    if (millis() - lastChangeTime > freezeTimeout) {
      digitalWrite(RELAY_HEATER1, LOW);
      digitalWrite(RELAY_HEATER2, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor Frozen!");
      lcd.setCursor(0, 1);
      lcd.print("Rebooting...");

      delay(1000);                // Show message for 1 second
      wdt_enable(WDTO_15MS);      // Enable watchdog reset
      while (1);                  // Wait for reset
    }

    // 🔹 Heater control
    if (tempGroup1 > 0) {
      if (tempGroup1 < targetTemp - hysteresis)
        digitalWrite(RELAY_HEATER1, HIGH);
      else if (tempGroup1 > targetTemp + hysteresis)
        digitalWrite(RELAY_HEATER1, LOW);
    }

    if (tempGroup2 > 0) {
      if (tempGroup2 < targetTemp - hysteresis)
        digitalWrite(RELAY_HEATER2, HIGH);
      else if (tempGroup2 > targetTemp + hysteresis)
        digitalWrite(RELAY_HEATER2, LOW);
    }

    // 🔹 Fogger control
    if (avgHumidity < targetHumidity - hysteresis)
      digitalWrite(RELAY_FOGGER, HIGH);
    else if (avgHumidity > targetHumidity + hysteresis)
      digitalWrite(RELAY_FOGGER, LOW);

    // 🔹 LCD display
    lcd.setCursor(0, 0);
    lcd.print("T1:");
    lcd.print(tempGroup1, 1);
    lcd.print(" T2:");
    lcd.print(tempGroup2, 1);

    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print(avgHumidity, 1);
    lcd.print("%     ");

    // 🔹 LCD change tracking
    if (tempGroup1 != lastDisplayT1 || tempGroup2 != lastDisplayT2 || avgHumidity != lastDisplayH) {
      lastDisplayT1 = tempGroup1;
      lastDisplayT2 = tempGroup2;
      lastDisplayH  = avgHumidity;
      lastDisplayChange = millis();
    }
  }

  // 🔹 Display freeze reboot
  if (millis() - lastDisplayChange >= displayFreezeTimeout) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Display Frozen!");
    delay(500);
    wdt_enable(WDTO_15MS);
    while (1);
  }
}

// 🔹 Average (2 values)
float average(float a, float b) {
  int valid = 0;
  float sum = 0;
  if (!isnan(a)) { sum += a; valid++; }
  if (!isnan(b)) { sum += b; valid++; }
  return valid > 0 ? sum / valid : 0;
}

// 🔹 Average (4 values)
float average(float a, float b, float c, float d) {
  int valid = 0;
  float sum = 0;
  if (!isnan(a)) { sum += a; valid++; }
  if (!isnan(b)) { sum += b; valid++; }
  if (!isnan(c)) { sum += c; valid++; }
  if (!isnan(d)) { sum += d; valid++; }
  return valid > 0 ? sum / valid : 0;
}
