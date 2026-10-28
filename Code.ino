
#include <ESP32Servo.h>

// ---------------- PIN CONFIGURATION ----------------
const int IR_PIN = 27;
const int SERVO_PIN = 18;

// Change this if your sensor is active HIGH instead of LOW
const bool ACTIVE_LOW = true;

// ---------------- SERVO CONFIGURATION ----------------
Servo dripServo;

const int SERVO_SLOW_ANGLE = 90;  // Intended to tighten clamp
const int SERVO_FAST_ANGLE = 45;  // Intended to loosen clamp

// Start at the neutral position
const int SERVO_START_ANGLE = 90;

// ---------------- DRIP RATE CONFIGURATION ----------------
float targetRate = 30.0;          // Target drops per minute
const float RATE_TOLERANCE = 3.0; // Acceptable error

const unsigned long SAMPLE_MS = 10000;
const unsigned long DEBOUNCE_MS = 100;

// ---------------- DROP COUNTING ----------------
unsigned long totalDrops = 0;
unsigned long dropsThisWindow = 0;

unsigned long lastDetectionTime = 0;
unsigned long lastSampleTime = 0;

bool previousActive = false;

// ---------------- STATE ----------------
int servoAngle = SERVO_START_ANGLE;
float dripRate = 0.0;

void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);

  dripServo.setPeriodHertz(50);
  dripServo.attach(SERVO_PIN, 500, 2400);
  dripServo.write(SERVO_START_ANGLE);

  lastSampleTime = millis();

  Serial.println();
  Serial.println("IV Drip Flow Control - Water Prototype");
  Serial.println("Target rate: 30 drops/minute");
  Serial.println("High rate -> 90 degrees");
  Serial.println("Low rate -> 45 degrees");
  Serial.println("Time_s,Drops,Rate_dpm,ServoAngle,Status");
}

void loop() {
  unsigned long now = millis();

  // -------- IR SENSOR DROP DETECTION --------
  bool active;

  if (ACTIVE_LOW) {
    active = (digitalRead(IR_PIN) == LOW);
  } else {
    active = (digitalRead(IR_PIN) == HIGH);
  }

  // Count one drop on the inactive-to-active transition
  if (active &&
      !previousActive &&
      (now - lastDetectionTime >= DEBOUNCE_MS)) {

    lastDetectionTime = now;
    totalDrops++;
    dropsThisWindow++;

    Serial.print("Drop detected. Total = ");
    Serial.println(totalDrops);
  }

  previousActive = active;

  // -------- RATE CALCULATION EVERY 10 SECONDS --------
  if (now - lastSampleTime >= SAMPLE_MS) {

    float elapsedSeconds =
        (now - lastSampleTime) / 1000.0;

    lastSampleTime = now;

    dripRate =
        dropsThisWindow * 60.0 / elapsedSeconds;

    float error = targetRate - dripRate;
    String status;

    // -------- TWO-POSITION CONTROL --------
    if (error < -RATE_TOLERANCE) {
      // Rate is too high: intended to slow flow
      servoAngle = SERVO_SLOW_ANGLE;
      status = "RATE HIGH - SLOW";
    }
    else if (error > RATE_TOLERANCE) {
      // Rate is too low: intended to increase flow
      servoAngle = SERVO_FAST_ANGLE;
      status = "RATE LOW - FAST";
    }
    else {
      // Rate is within target range: hold current position
      status = "RATE OK - HOLD";
    }

    dripServo.write(servoAngle);

    // -------- SERIAL OUTPUT --------
    Serial.print(now / 1000.0, 1);
    Serial.print(",");
    Serial.print(dropsThisWindow);
    Serial.print(",");
    Serial.print(dripRate, 1);
    Serial.print(",");
    Serial.print(servoAngle);
    Serial.print(",");
    Serial.println(status);

    // Reset the count for the next window
    dropsThisWindow = 0;
  }

  delay(2);
}
