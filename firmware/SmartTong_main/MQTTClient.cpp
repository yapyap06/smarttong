/**
 * SmartTONG AI — MQTTClient.cpp  v2.0
 */

#include "MQTTClient.h"

// ─── begin ────────────────────────────────────────────────────────────────────
void MQTTClient::begin() {
  Serial.println("[Net] MQTTClient v2.0 ready.");
}

// ─── connectWiFi ─────────────────────────────────────────────────────────────
bool MQTTClient::connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[Net] Connecting to WiFi: %s", WIFI_SSID);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[Net] WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println("[Net] WiFi FAILED — will retry on next cycle.");
  return false;
}

// ─── isConnected ─────────────────────────────────────────────────────────────
bool MQTTClient::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// ─── buildJSON ───────────────────────────────────────────────────────────────
String MQTTClient::buildJSON(const BinPayload& p) {
  StaticJsonDocument<512> doc;
  doc["binId"]           = p.binId;
  doc["fillPercent"]     = round(p.fillPercent * 10) / 10.0;
  doc["gasLevel"]        = p.gasLevel;
  doc["gasAlert"]        = p.gasAlert;
  doc["lastWasteType"]   = p.lastWasteType;
  doc["wasteConfidence"] = round(p.wasteConfidence * 100) / 100.0;
  doc["hazardous"]       = p.hazardous;
  doc["lidOpenCount"]    = p.lidOpenCount;
  doc["lat"]             = p.lat;
  doc["lng"]             = p.lng;
  doc["battPercent"]     = p.battPercent;   // Phase 1
  doc["battLow"]         = p.battLow;       // Phase 1
  doc["timestamp"]       = getISOTimestamp();

  String out;
  serializeJson(doc, out);
  return out;
}

// ─── buildSupabaseJSON ───────────────────────────────────────────────────────
// Supabase REST /rest/v1/{table} expects column-named fields
String MQTTClient::buildSupabaseJSON(const BinPayload& p) {
  StaticJsonDocument<512> doc;
  doc["bin_id"]          = p.binId;
  doc["fill_percent"]    = p.fillPercent;
  doc["gas_level"]       = p.gasLevel;
  doc["gas_alert"]       = p.gasAlert;
  doc["waste_type"]      = p.lastWasteType;
  doc["hazardous"]       = p.hazardous;
  doc["lid_count"]       = p.lidOpenCount;
  doc["lat"]             = p.lat;
  doc["lng"]             = p.lng;
  doc["batt_percent"]    = p.battPercent;
  doc["batt_low"]        = p.battLow;
  doc["recorded_at"]     = getISOTimestamp();

  String out;
  serializeJson(doc, out);
  return out;
}

// ─── publish (Firebase RTDB) ─────────────────────────────────────────────────
bool MQTTClient::publish(const BinPayload& payload) {
  if (!isConnected()) {
    Serial.println("[Net] publish skipped — not connected.");
    return false;
  }

  String url  = String(FIREBASE_DB_URL) + "/readings/" + payload.binId + ".json";
  String body = buildJSON(payload);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(body);   // PUT = overwrite (latest snapshot) per bin

  Serial.printf("[Net] Firebase PUT %s → HTTP %d\n", payload.binId, code);

  if (code == 200) {
    // Also write to /history/{binId}/readings collection (append)
    String histUrl = String(FIREBASE_DB_URL) + "/history/" + payload.binId
                   + "/" + String(millis()) + ".json";
    HTTPClient hh;
    hh.begin(histUrl);
    hh.addHeader("Content-Type", "application/json");
    hh.PUT(body);
    hh.end();
  }

  http.end();

  // Phase 2: optionally dual-publish to Supabase
  publishToSupabase(payload);

  return (code == 200 || code == 201);
}

// ─── publishToSupabase ───────────────────────────────────────────────────────
bool MQTTClient::publishToSupabase(const BinPayload& payload) {
  if (strlen(SUPABASE_URL) == 0 || strlen(SUPABASE_KEY) == 0) {
    return false;  // Not configured — silent skip
  }

  String url  = String(SUPABASE_URL) + "/rest/v1/" + String(SUPABASE_TABLE);
  String body = buildSupabaseJSON(payload);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Prefer", "return=minimal");

  int code = http.POST(body);
  Serial.printf("[Net] Supabase POST → HTTP %d\n", code);
  http.end();
  return (code == 201);
}

// ─── pollCommand ─────────────────────────────────────────────────────────────
// Polls Firebase RTDB /commands/{binId}/pendingCommand for 5 seconds.
// If a command token is found, clears it and returns the command type.
// This lets the cloud (commandDispatch CF) remotely unlock a slot.
PendingCommand MQTTClient::pollCommand() {
  if (!isConnected()) return CMD_NONE;

  String url = String(FIREBASE_DB_URL) + "/commands/" + BIN_ID + "/pendingCommand.json";

  HTTPClient http;
  http.begin(url);
  int code = http.GET();

  if (code != 200) {
    http.end();
    return CMD_NONE;
  }

  String payload = http.getString();
  http.end();

  // Firebase returns null (no command) or a quoted string like "OPEN_SLOT_A"
  payload.trim();
  if (payload == "null" || payload.length() == 0) return CMD_NONE;

  Serial.printf("[Net] Command received: %s\n", payload.c_str());

  // Clear the command immediately so it doesn't re-trigger
  HTTPClient delHttp;
  delHttp.begin(url);
  delHttp.addHeader("Content-Type", "application/json");
  delHttp.PUT("null");
  delHttp.end();

  if (payload.indexOf("OPEN_SLOT_A") >= 0) return CMD_OPEN_A;
  if (payload.indexOf("OPEN_SLOT_B") >= 0) return CMD_OPEN_B;
  if (payload.indexOf("LOCK")        >= 0) return CMD_LOCK;
  return CMD_NONE;
}

// ─── getISOTimestamp ─────────────────────────────────────────────────────────
String MQTTClient::getISOTimestamp() {
  // Basic ISO-8601 using millis offset from boot (NTP not required for RTDB)
  // For real deployment: add NTP sync in begin() and use configTime()
  unsigned long sec = millis() / 1000;
  char buf[32];
  snprintf(buf, sizeof(buf), "T+%lus", sec);
  return String(buf);
}
