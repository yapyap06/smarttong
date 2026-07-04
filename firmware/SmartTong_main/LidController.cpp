/**
 * SmartTONG AI — LidController.cpp
 */

#include "LidController.h"

void LidController::begin() {
  _servo.attach(SERVO_PIN);
  _servo.write(SERVO_CLOSED_DEG);
  _isOpen = false;
  Serial.println("[LidController] Servo attached, lid CLOSED.");
}

void LidController::update() {
  bool pirHigh = digitalRead(PIR_PIN) == HIGH;

  if (pirHigh && !_isOpen) {
    forceOpen();
  }

  if (_isOpen && (millis() - _openedAt >= LID_OPEN_DURATION_MS)) {
    forceClose();
  }
}

bool LidController::isOpen() const {
  return _isOpen;
}

void LidController::forceOpen() {
  _servo.write(SERVO_OPEN_DEG);
  _isOpen   = true;
  _openedAt = millis();
  Serial.println("[LidController] Lid OPEN.");
}

void LidController::forceClose() {
  _servo.write(SERVO_CLOSED_DEG);
  _isOpen = false;
  Serial.println("[LidController] Lid CLOSED.");
}
