/**
 * SmartTONG AI — DeepSleepManager.cpp  (Phase 2)
 */

#include "DeepSleepManager.h"
#include <Arduino.h>
#include "esp_sleep.h"

// ─── RTC-backed state (survives deep sleep) ───────────────────────────────────
RTC_DATA_ATTR uint32_t rtcLidCount       = 0;
RTC_DATA_ATTR float    rtcLastFillPercent = 0.0f;
RTC_DATA_ATTR uint8_t  rtcLastBattPercent = 100;
RTC_DATA_ATTR uint32_t rtcBootCount      = 0;

// ─── begin ────────────────────────────────────────────────────────────────────
void DeepSleepManager::begin() {
  rtcBootCount++;
  printWakeReason();
  Serial.printf("[Sleep] Boot #%lu | lidCount=%lu | lastFill=%.0f%% | batt=%d%%\n",
                rtcBootCount, rtcLidCount, rtcLastFillPercent, rtcLastBattPercent);
}

// ─── wasWokenByTimer ─────────────────────────────────────────────────────────
bool DeepSleepManager::wasWokenByTimer() {
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

// ─── wasWokenByPIR ───────────────────────────────────────────────────────────
bool DeepSleepManager::wasWokenByPIR() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  return (cause == ESP_SLEEP_WAKEUP_EXT0 || cause == ESP_SLEEP_WAKEUP_EXT1);
}

// ─── configPIRWakeup ─────────────────────────────────────────────────────────
// Uses EXT1 wakeup so the PIR can wake from deep sleep.
// EXT1 requires a bitmask of GPIO pins; PIR_PIN must be in GPIO 0-39.
void DeepSleepManager::configPIRWakeup() {
  uint64_t pirMask = (1ULL << PIR_PIN);
  esp_sleep_enable_ext1_wakeup(pirMask, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.printf("[Sleep] EXT1 wakeup armed on GPIO%d (PIR)\n", PIR_PIN);
}

// ─── sleep ───────────────────────────────────────────────────────────────────
void DeepSleepManager::sleep() {
  Serial.printf("[Sleep] Entering deep sleep for %llu s...\n",
                DEEP_SLEEP_INTERVAL_US / 1000000ULL);
  Serial.flush();
  delay(100);  // Let UART drain

  // Enable timer wakeup
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US);

  // Configure PIR as additional wakeup source
  configPIRWakeup();

  esp_deep_sleep_start();
  // ── never reaches here ──
}

// ─── saveState ───────────────────────────────────────────────────────────────
void DeepSleepManager::saveState(uint32_t lidCount, float fillPct, uint8_t battPct) {
  rtcLidCount        = lidCount;
  rtcLastFillPercent = fillPct;
  rtcLastBattPercent = battPct;
}

// ─── printWakeReason ─────────────────────────────────────────────────────────
void DeepSleepManager::printWakeReason() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER:   Serial.println("[Sleep] Wake reason: TIMER (heartbeat)"); break;
    case ESP_SLEEP_WAKEUP_EXT0:    Serial.println("[Sleep] Wake reason: EXT0 (PIR motion)"); break;
    case ESP_SLEEP_WAKEUP_EXT1:    Serial.println("[Sleep] Wake reason: EXT1 (PIR motion)"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:Serial.println("[Sleep] Wake reason: TOUCHPAD");           break;
    default:                       Serial.println("[Sleep] Wake reason: POWER-ON / RESET");  break;
  }
}
