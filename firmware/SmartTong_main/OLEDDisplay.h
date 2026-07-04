/**
 * SmartTONG AI — OLEDDisplay.h  (Phase 1)
 *
 * Drives a 0.96" SSD1306 128×64 OLED via I2C (pins in Config.h).
 * Requires: Adafruit SSD1306 + Adafruit GFX library (install via Library Manager).
 *
 * Display layout:
 *   Line 0 (big):  SmartTONG AI  [WiFi icon]
 *   Line 1:        Fill: 72%   Bat: 85%
 *   Line 2:        Type: PLASTIK
 *   Line 3:        Status bar (fill level visual)
 *   Line 4 (small): B01 · Shah Alam
 */

#pragma once
#include "Config.h"

// Forward-declare Adafruit types to avoid pulling heavy headers into Config.h
// The full #include is in the .cpp where it belongs.
class Adafruit_SSD1306;

class OLEDDisplay {
public:
  void begin();

  // ── Core update (called after every sensor read) ─────────────────────
  void update(float fillPct, uint8_t battPct, const char* wasteType,
              bool wifiOk, bool isHazardous);

  // ── Startup splash (3-second branding screen) ────────────────────────
  void showSplash();

  // ── Power management ─────────────────────────────────────────────────
  void dimCheck(unsigned long nowMs);   // Auto-dim after OLED_DIM_TIMEOUT_MS
  void wakeDisplay();                   // Re-activate on lid open event

private:
  Adafruit_SSD1306* _display = nullptr;
  unsigned long     _lastActivityMs = 0;
  bool              _dimmed = false;

  void _drawFillBar(float pct, bool hazardous);
};
