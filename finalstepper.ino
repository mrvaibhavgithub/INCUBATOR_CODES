#define PUL 2
#define DIR 3

const int microstep = 4;
const int stepsPerRotation = 200 * microstep;
const int rotations = 10;
const int totalSteps = stepsPerRotation * rotations;

const int stepDelay = 1200;
const unsigned long pauseDuration = 3600000UL;

unsigned long previousMillis = 0;
bool motorDirection = HIGH;
bool isMoving = true;
int currentStep = 0;

void rotateMotorStep() {
  digitalWrite(PUL, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(PUL, LOW);
  delayMicroseconds(stepDelay);
  currentStep++;
}

void setup() {
  pinMode(PUL, OUTPUT);
  pinMode(DIR, OUTPUT);
  digitalWrite(DIR, motorDirection);
  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (isMoving) {
    if (currentStep < totalSteps) {
      rotateMotorStep();
    } else {
      isMoving = false;
      previousMillis = currentMillis;
    }
  } else {
    if (currentMillis - previousMillis >= pauseDuration) {
      motorDirection = !motorDirection;
      digitalWrite(DIR, motorDirection);
      currentStep = 0;
      isMoving = true;
    }
  }
}