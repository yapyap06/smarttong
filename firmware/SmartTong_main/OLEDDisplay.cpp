/**
 * SmartTONG AI — OLEDDisplay.cpp  (Phase 1)
 *
 * Library dependencies (Arduino IDE → Library Manager):
 *   • Adafruit SSD1306   by Adafruit
 *   • Adafruit GFX Library by Adafruit
 */

#include "OLEDDisplay.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── begin ────────────────────────────────────────────────────────────────────
void OLEDDisplay::begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  _display = new Adafruit_SSD1306(OLED_SCREEN_W, OLED_SCREEN_H, &Wire, -1);

  if (!_display->begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("[OLED] SSD1306 not found — check wiring / I2C address.");
    delete _display;
    _display = nullptr;
    return;
  }

  _display->clearDisplay();
  _display->setTextColor(SSD1306_WHITE);
  Serial.println("[OLED] SSD1306 ready.");
  showSplash();
}

// ─── showSplash ──────────────────────────────────────────────────────────────
void OLEDDisplay::showSplash() {
  if (!_display) return;
  _display->clearDisplay();

  // Big logo text
  _display->setTextSize(2);
  _display->setCursor(4, 4);
  _display->print("SmartTONG");

  _display->setTextSize(1);
  _display->setCursor(4, 26);
  _display->print("AI Sistem Sisa Pintar");

  _display->setCursor(4, 38);
  _display->print("AINS 2026 Selangor");

  // Bottom tag
  _display->setCursor(4, 54);
  _display->print("Booting...");

  _display->display();
  delay(2500);
}

// ─── update ──────────────────────────────────────────────────────────────────
void OLEDDisplay::update(float fillPct, uint8_t battPct, const char* wasteType,
                          bool wifiOk, bool isHazardous) {
  if (!_display) return;
  _display->clearDisplay();
  _lastActivityMs = millis();
  _dimmed = false;
  _display->ssd1306_command(SSD1306_DISPLAYON);

  // ── Row 0: Header + WiFi indicator ──────────────────────────────────
  _display->setTextSize(1);
  _display->setCursor(0, 0);
  _display->print("SmartTONG AI");
  _display->setCursor(90, 0);
  _display->print(wifiOk ? "WiFi:OK" : "WiFi:--");

  // ── Row 1: Divider ──────────────────────────────────────────────────
  _display->drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // ── Row 2: Fill % (large) + Battery % ───────────────────────────────
  _display->setTextSize(2);
  _display->setCursor(0, 14);
  _display->printf("%.0f%%", fillPct);

  _display->setTextSize(1);
  _display->setCursor(70, 14);
  _display->printf("Bat:%d%%", battPct);

  // Battery bar (small, top-right)
  _display->drawRect(70, 25, 40, 7, SSD1306_WHITE);
  int barW = (int)(battPct * 38 / 100);
  _display->fillRect(71, 26, barW, 5, SSD1306_WHITE);
  // Battery terminal nub
  _display->fillRect(110, 27, 3, 3, SSD1306_WHITE);

  // ── Row 3: Fill progress bar ────────────────────────────────────────
  _display->drawRect(0, 34, 62, 8, SSD1306_WHITE);
  int fillBarW = (int)(fillPct * 60 / 100);
  _display->fillRect(1, 35, fillBarW, 6, SSD1306_WHITE);

  // ── Row 4: Waste type + hazard flag ────────────────────────────────
  _display->setTextSize(1);
  _display->setCursor(0, 46);
  if (isHazardous) {
    _display->print("! BAHAYA: ");
    _display->print(wasteType);
  } else {
    _display->print("Jenis: ");
    _display->print(wasteType);
  }

  // ── Row 5: Bin ID + location ────────────────────────────────────────
  _display->setCursor(0, 56);
  _display->printf("%s  %.4f,%.4f", BIN_ID, BIN_LOCATION_LAT, BIN_LOCATION_LNG);

  _display->display();
}

// ─── dimCheck ────────────────────────────────────────────────────────────────
void OLEDDisplay::dimCheck(unsigned long nowMs) {
  if (!_display || _dimmed) return;
  if (nowMs - _lastActivityMs >= OLED_DIM_TIMEOUT_MS) {
    _display->ssd1306_command(SSD1306_DISPLAYOFF);
    _dimmed = true;
    Serial.println("[OLED] Display dimmed.");
  }
}

// ─── wakeDisplay ─────────────────────────────────────────────────────────────
void OLEDDisplay::wakeDisplay() {
  if (!_display) return;
  _display->ssd1306_command(SSD1306_DISPLAYON);
  _dimmed = false;
  _lastActivityMs = millis();
}
