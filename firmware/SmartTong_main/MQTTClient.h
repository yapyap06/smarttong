/**
 * SmartTONG AI — MQTTClient.h  v2.0
 *
 * WiFi + HTTP publish to Firebase Realtime Database.
 * Phase 2 additions:
 *   • battPercent / battLow fields in BinPayload
 *   • publishToSupabase() — dual-backend REST path (optional)
 *   • pollCommand()       — polls Firebase RTDB for pendingCommand flag
 *                           (enables remote slot unlock via commandDispatch CF)
 */

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "SensorHub.h"
#include "CameraAI.h"

// ─── Payload struct (extended with battery fields) ────────────────────────────
struct BinPayload {
  const char*   binId;
  float         fillPercent;
  int           gasLevel;
  bool          gasAlert;
  const char*   lastWasteType;
  float         wasteConfidence;
  bool          hazardous;
  int           lidOpenCount;
  float         lat;
  float         lng;
  uint8_t       battPercent;   // NEW (Phase 1)
  bool          battLow;       // NEW (Phase 1)
};

// ─── Command type from commandDispatch Cloud Function ────────────────────────
enum PendingCommand {
  CMD_NONE      = 0,
  CMD_OPEN_A    = 1,   // Open Slot A (e.g. recyclables slot)
  CMD_OPEN_B    = 2,   // Open Slot B (e.g. general waste slot)
  CMD_LOCK      = 3,   // Lock all slots
};

class MQTTClient {
public:
  void begin();
  bool connectWiFi();

  // ── Publish telemetry ───────────────────────────────────────────────
  bool publish(const BinPayload& payload);

  // ── Phase 2: dual-publish to Supabase REST (no-op if SUPABASE_URL empty)
  bool publishToSupabase(const BinPayload& payload);

  // ── Phase 2: poll Firebase RTDB for pending actuation command ──────
  // Call this after a lid-close event. Dispatches lid if CMD received.
  PendingCommand pollCommand();

  bool isConnected();
  void incrementLidCount() { _lidOpenCount++; }
  int  getLidCount()       { return _lidOpenCount; }

private:
  String buildJSON(const BinPayload& p);
  String buildSupabaseJSON(const BinPayload& p);
  String getISOTimestamp();
  int    _lidOpenCount = 0;
};
