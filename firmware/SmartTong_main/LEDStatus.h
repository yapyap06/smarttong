/**
 * SmartTONG AI — LEDStatus.h
 * NeoPixel RGB LED showing fill-level status.
 * Green (<50%) | Amber (50–80%) | Red (>80%) | Purple (Hazardous)
 */

#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

class LEDStatus {
public:
  void begin();
  void setFill(float fillPercent, bool hazardous = false);
  void blink(uint32_t color, int times = 3, int delayMs = 200);
  void off();

private:
  Adafruit_NeoPixel _strip;
};
