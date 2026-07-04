/**
 * SmartTONG AI — BatteryMonitor.cpp  (Phase 1)
 */

#include "BatteryMonitor.h"
#include <Arduino.h>

void BatteryMonitor::begin() {
  analogSetAttenuation(ADC_11db);   // full 3.3V range on all ADC1 pins
  pinMode(BATTERY_ADC_PIN, INPUT);
  // Warm-up: discard first few readings (ADC needs a moment after WiFi init)
  for (int i = 0; i < 5; i++) {
    analogRead(BATTERY_ADC_PIN);
    delay(5);
  }
  Serial.printf("[Battery] ADC pin %d ready. Initial: %.2fV (%d%%)\n",
                BATTERY_ADC_PIN, readVoltage(), readPercent());
}

// ─── Private: 8-sample averaged ADC read ──────────────────────────────────
float BatteryMonitor::_averaged() {
  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(BATTERY_ADC_PIN);
    delay(2);
  }
  return (float)(sum / 8);
}

// ─── readVoltage ──────────────────────────────────────────────────────────
float BatteryMonitor::readVoltage() {
  float adcRaw     = _averaged();
  float adcVoltage = adcRaw / (float)BATTERY_ADC_BITS * BATTERY_ADC_REF_V;
  float vbat       = adcVoltage * (BATTERY_VDIV_R1 + BATTERY_VDIV_R2) / BATTERY_VDIV_R2;
  _lastVoltage = vbat;
  return vbat;
}

// ─── readPercent ──────────────────────────────────────────────────────────
// Piecewise linear approximation of a typical 18650 discharge curve.
// Full curve: 4.20V=100%, 4.00V=80%, 3.80V=60%, 3.60V=40%, 3.40V=20%, 3.00V=0%
uint8_t BatteryMonitor::readPercent() {
  float v = readVoltage();

  float pct;
  if      (v >= 4.20f) pct = 100.0f;
  else if (v >= 4.00f) pct = 80.0f + (v - 4.00f) / 0.20f * 20.0f;
  else if (v >= 3.80f) pct = 60.0f + (v - 3.80f) / 0.20f * 20.0f;
  else if (v >= 3.60f) pct = 40.0f + (v - 3.60f) / 0.20f * 20.0f;
  else if (v >= 3.40f) pct = 20.0f + (v - 3.40f) / 0.20f * 20.0f;
  else if (v >= 3.00f) pct =  0.0f + (v - 3.00f) / 0.40f * 20.0f;
  else                 pct =  0.0f;

  _lastPercent = (uint8_t)constrain(pct, 0.0f, 100.0f);
  return _lastPercent;
}

// ─── isLow ────────────────────────────────────────────────────────────────
bool BatteryMonitor::isLow() {
  return (_lastPercent < BATTERY_LOW_PERCENT);
}

// ─── printStatus ──────────────────────────────────────────────────────────
void BatteryMonitor::printStatus() {
  Serial.printf("[Battery] %.2fV | %d%% %s\n",
                _lastVoltage, _lastPercent,
                isLow() ? "⚠ LOW!" : "OK");
}
