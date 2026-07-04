/**
 * SmartTONG AI — DeepSleepManager.h  (Phase 2)
 *
 * Manages deep sleep wake/sleep cycles for real-world deployment.
 * Only active when DEMO_MODE == 0 (see Config.h).
 *
 * Wake sources:
 *   • Timer (ULP periodic): heartbeat telemetry every 30 min
 *   • EXT1 GPIO (PIR motion): instant wake on user approach
 *
 * RTC memory: persists lidCount and lastFillPercent across sleep cycles
 * so we don't lose state between wakeups.
 */

#pragma once
#include "Config.h"

// RTC-backed variables survive deep sleep (stored in 8kB RTC SRAM)
RTC_DATA_ATTR extern uint32_t rtcLidCount;
RTC_DATA_ATTR extern float    rtcLastFillPercent;
RTC_DATA_ATTR extern uint8_t  rtcLastBattPercent;
RTC_DATA_ATTR extern uint32_t rtcBootCount;

class DeepSleepManager {
public:
  // ── Call once in setup() to log wake reason ─────────────────────────
  void begin();

  // ── Returns why we woke up ──────────────────────────────────────────
  bool wasWokenByTimer();  // true = scheduled heartbeat
  bool wasWokenByPIR();    // true = motion detected

  // ── Configure PIR GPIO as EXT1 wakeup source ────────────────────────
  void configPIRWakeup();

  // ── Enter deep sleep (blocks — ESP32 restarts on wake) ─────────────
  void sleep();

  // ── Convenience: persist current readings to RTC memory ─────────────
  void saveState(uint32_t lidCount, float fillPct, uint8_t battPct);

  // ── Print wake reason to Serial ─────────────────────────────────────
  void printWakeReason();
};
