/**
 * SmartTONG AI — BatteryMonitor.h  (Phase 1)
 *
 * Reads 18650 Li-ion cell voltage via a resistor voltage divider:
 *   VBAT ─── R1 (100kΩ) ─┬─ GPIO35 (ADC1_CH7)
 *                          └─ R2 (47kΩ) ─── GND
 *
 * VBAT = ADC_reading/4095 × 3.3V × (R1+R2)/R2
 * Battery % maps the LiPo discharge curve (4.2V → 0%, 3.0V → 100%).
 *
 * NOTE: Always use ADC1 pins (32–39) with WiFi active.
 *       ADC2 is shared with the WiFi radio and gives garbage readings.
 */

#pragma once
#include "Config.h"

class BatteryMonitor {
public:
  void     begin();
  float    readVoltage();      // Returns raw cell voltage (V)
  uint8_t  readPercent();      // Returns 0–100 %
  bool     isLow();            // True when % < BATTERY_LOW_PERCENT
  void     printStatus();      // Serial debug dump

private:
  uint8_t  _lastPercent = 100;
  float    _lastVoltage = 4.2f;

  // Smooth the ADC with a simple 8-sample average to reject noise
  float    _averaged();
};
