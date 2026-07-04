/**
 * SmartTONG AI — LEDStatus.cpp
 */

#include "LEDStatus.h"

void LEDStatus::begin() {
  _strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
  _strip.begin();
  _strip.setBrightness(LED_BRIGHTNESS);
  _strip.clear();
  _strip.show();
  Serial.println("[LEDStatus] NeoPixel initialized.");
}

void LEDStatus::setFill(float fillPercent, bool hazardous) {
  uint32_t color;
  if (hazardous) {
    color = _strip.Color(128, 0, 128);   // Purple — hazardous
    Serial.println("[LEDStatus] PURPLE — hazardous");
  } else if (fillPercent >= FILL_RED_PERCENT) {
    color = _strip.Color(255, 0, 0);     // Red — full
    Serial.printf("[LEDStatus] RED — %.0f%%\n", fillPercent);
  } else if (fillPercent >= FILL_AMBER_PERCENT) {
    color = _strip.Color(255, 100, 0);   // Amber — half full
    Serial.printf("[LEDStatus] AMBER — %.0f%%\n", fillPercent);
  } else {
    color = _strip.Color(0, 200, 50);    // Green — ok
    Serial.printf("[LEDStatus] GREEN — %.0f%%\n", fillPercent);
  }
  _strip.setPixelColor(0, color);
  _strip.show();
}

void LEDStatus::blink(uint32_t color, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    _strip.setPixelColor(0, color);
    _strip.show();
    delay(delayMs);
    _strip.clear();
    _strip.show();
    delay(delayMs);
  }
}

void LEDStatus::off() {
  _strip.clear();
  _strip.show();
}
