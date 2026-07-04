/**
 * SmartTONG AI — SensorHub.h
 * Manages all physical sensors: ultrasonic fill-level, MQ-135 gas, PIR.
 */

#pragma once
#include <Arduino.h>
#include "Config.h"

struct SensorReading {
  float  fillPercent;    // 0.0 – 100.0
  float  distanceCm;     // raw ultrasonic distance
  int    gasLevel;       // raw ADC 0–4095
  bool   pirTriggered;   // true if motion detected this cycle
  bool   gasAlert;       // true if gas exceeds threshold
};

class SensorHub {
public:
  void begin();
  SensorReading read();

private:
  float measureDistanceCm();
  float distanceToFillPercent(float distCm);
};
