/**
 * SmartTONG AI — SensorHub.cpp
 */

#include "SensorHub.h"

void SensorHub::begin() {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  // MQ-135: analog input, no setup needed beyond ADC
  Serial.println("[SensorHub] Initialized.");
}

SensorReading SensorHub::read() {
  SensorReading r;
  r.distanceCm  = measureDistanceCm();
  r.fillPercent = distanceToFillPercent(r.distanceCm);
  r.gasLevel    = analogRead(GAS_SENSOR_PIN);
  r.pirTriggered = digitalRead(PIR_PIN) == HIGH;
  r.gasAlert    = r.gasLevel > GAS_ALERT_THRESHOLD;

  Serial.printf("[SensorHub] dist=%.1fcm fill=%.0f%% gas=%d pir=%d\n",
                r.distanceCm, r.fillPercent, r.gasLevel, r.pirTriggered);
  return r;
}

float SensorHub::measureDistanceCm() {
  // Trigger pulse
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  // Read echo (timeout 30ms = ~5m max range)
  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000);
  if (duration == 0) return BIN_EMPTY_CM;  // timeout = nothing detected = empty

  float distance = duration * 0.0343f / 2.0f;  // µs → cm

  // Clamp to valid range
  if (distance < BIN_FULL_CM)  distance = BIN_FULL_CM;
  if (distance > BIN_EMPTY_CM) distance = BIN_EMPTY_CM;
  return distance;
}

float SensorHub::distanceToFillPercent(float distCm) {
  // Map: BIN_EMPTY_CM → 0%, BIN_FULL_CM → 100%
  float pct = (BIN_EMPTY_CM - distCm) / (BIN_EMPTY_CM - BIN_FULL_CM) * 100.0f;
  if (pct < 0.0f)   pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  return pct;
}
