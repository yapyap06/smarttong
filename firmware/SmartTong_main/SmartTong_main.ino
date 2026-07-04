/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║           SmartTONG AI — Main Firmware Sketch  v2.0                  ║
 * ║           ESP32 DevKit V1 + ESP32-CAM (optional)                     ║
 * ║                                                                       ║
 * ║  NEW in v2.0 (Phase 1 & 2):                                          ║
 * ║   • BatteryMonitor  — 18650 voltage divider → battery %              ║
 * ║   • OLEDDisplay     — SSD1306 0.96" status screen                    ║
 * ║   • DeepSleepManager— 30-min deep sleep + PIR EXT1 wakeup            ║
 * ║   • DEMO_MODE flag  — always-on for judging booth                     ║
 * ║   • Supabase dual-publish path                                        ║
 * ║   • commandDispatch webhook polling (remote slot unlock)              ║
 * ║                                                                       ║
 * ║  Libraries required (install via Arduino Library Manager):           ║
 * ║   • Adafruit NeoPixel       by Adafruit                              ║
 * ║   • Adafruit SSD1306        by Adafruit                              ║
 * ║   • Adafruit GFX Library    by Adafruit                              ║
 * ║   • ESP32Servo              by Kevin Harrington                      ║
 * ║   • ArduinoJson             by Benoit Blanchon (v6)                  ║
 * ║                                                                       ║
 * ║  For Wokwi: paste code into wokwi.com, diagram.json in same project  ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

#include <Arduino.h>
#include "Config.h"
#include "SensorHub.h"
#include "CameraAI.h"
#include "LidController.h"
#include "LEDStatus.h"
#include "OKUAudio.h"
#include "MQTTClient.h"
#include "BatteryMonitor.h"
#include "OLEDDisplay.h"
#include "DeepSleepManager.h"

// ─── Module instances ────────────────────────────────────────────────────────
SensorHub        sensors;
CameraAI         camera;
LidController    lid;
LEDStatus        led;
OKUAudio         audio;
MQTTClient       net;
BatteryMonitor   battery;
OLEDDisplay      oled;
DeepSleepManager sleepMgr;

// ─── State ───────────────────────────────────────────────────────────────────
SensorReading     lastReading;
ClassificationResult lastClass;

unsigned long lastPublishMs   = 0;
unsigned long lastEventMs     = 0;
bool          lidWasOpen      = false;
bool          eventPending    = false;

// ─── Helper: build and publish BinPayload ────────────────────────────────────
void publishPayload() {
  uint8_t battPct = battery.readPercent();

  BinPayload p = {
    BIN_ID,
    lastReading.fillPercent,
    lastReading.gasLevel,
    lastReading.gasAlert,
    lastClass.label,
    lastClass.confidence,
    (lastClass.type == WASTE_HAZARDOUS),
    net.getLidCount(),
    BIN_LOCATION_LAT,
    BIN_LOCATION_LNG,
    battPct,
    battery.isLow()
  };
  net.publish(p);

  // Update OLED
  oled.update(lastReading.fillPercent, battPct, lastClass.label,
               net.isConnected(), lastClass.type == WASTE_HAZARDOUS);

  // Persist to RTC for deep sleep wake continuity
  sleepMgr.saveState(net.getLidCount(), lastReading.fillPercent, battPct);
}

// ─── Startup ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔══════════════════════════════╗");
  Serial.println("║   SmartTONG AI  v2.0         ║");
  Serial.println("║   AINS 2026 Selangor         ║");
  Serial.printf( "║   Demo Mode: %-4s            ║\n", DEMO_MODE ? "ON" : "OFF");
  Serial.println("╚══════════════════════════════╝\n");

  // ── Phase 2: log wake reason (only meaningful after first boot) ──────
#if !DEMO_MODE
  sleepMgr.begin();
#endif

  // ── Init all modules ─────────────────────────────────────────────────
  sensors.begin();
  camera.begin();
  lid.begin();
  led.begin();
  audio.begin();
  battery.begin();
  oled.begin();    // Shows splash screen for 2.5s
  net.begin();

  // Start-up blink: blue = booting
  led.blink(Adafruit_NeoPixel::Color(0, 0, 200), 3, 150);

  // Connect WiFi
  net.connectWiFi();

  // Initial sensor + battery read
  lastReading = sensors.read();
  lastClass   = camera.classify();
  uint8_t battPct = battery.readPercent();

  // Update LED + OLED with initial state
  led.setFill(lastReading.fillPercent, lastClass.type == WASTE_HAZARDOUS);
  oled.update(lastReading.fillPercent, battPct, lastClass.label,
               net.isConnected(), lastClass.type == WASTE_HAZARDOUS);
  battery.printStatus();

  Serial.println("\n[Main] Setup complete.\n");

  // ── Phase 2: Deep Sleep fast-path (non-demo, timer wakeup) ──────────
#if !DEMO_MODE
  if (sleepMgr.wasWokenByTimer()) {
    // Heartbeat: read → publish → sleep
    Serial.println("[Main] TIMER wakeup — heartbeat publish.");
    lastPublishMs = 0; // Force immediate publish in loop
  } else if (sleepMgr.wasWokenByPIR()) {
    Serial.println("[Main] PIR wakeup — lid event flow.");
    // Falls through to main loop which handles the PIR→lid sequence
  }
#endif
}

// ─── Main loop ───────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // 1. Update lid state (PIR detection + auto-close timer)
  lid.update();

  // 2. Detect lid-open event → trigger camera + audio
  bool lidNowOpen = lid.isOpen();
  if (lidNowOpen && !lidWasOpen) {
    oled.wakeDisplay();
    net.incrementLidCount();
    lastReading = sensors.read();
    lastClass   = camera.classify();

    led.setFill(lastReading.fillPercent, lastClass.type == WASTE_HAZARDOUS);
    audio.playForFillLevel(lastReading.fillPercent, lastClass.type == WASTE_HAZARDOUS);

    eventPending = true;
    lastEventMs  = now;

    Serial.printf("[Main] LID OPEN EVENT — fill=%.0f%% type=%s bat=%d%%\n",
                  lastReading.fillPercent, lastClass.label, battery.readPercent());
  }

  if (!lidNowOpen && lidWasOpen) {
    audio.playThankYou();
    // Check for backend command (remote slot unlock from commandDispatch)
    net.pollCommand();
    Serial.println("[Main] Lid closed.");
  }
  lidWasOpen = lidNowOpen;

  // 3. Publish event (5s after lid opens) — gives sensor time to settle
  if (eventPending && (now - lastEventMs >= EVENT_PUBLISH_MS)) {
    publishPayload();
    eventPending = false;
    Serial.println("[Main] Event payload published.");
  }

  // 4. Periodic heartbeat publish every 10 minutes
  if (now - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;
    lastReading   = sensors.read();

    led.setFill(lastReading.fillPercent, lastClass.type == WASTE_HAZARDOUS);

    if (!net.isConnected()) net.connectWiFi();

    publishPayload();
    Serial.println("[Main] Heartbeat published.");

    // In deep sleep mode: after heartbeat publish, return to sleep
#if !DEMO_MODE
    if (sleepMgr.wasWokenByTimer()) {
      sleepMgr.sleep();
    }
#endif
  }

  // 5. Emergency: fill > 90% + gas → immediate alert (max once/min)
  if (lastReading.fillPercent > 90.0f && lastReading.gasAlert) {
    static unsigned long lastEmergencyMs = 0;
    if (now - lastEmergencyMs > 60000UL) {
      lastEmergencyMs = now;
      Serial.println("[Main] ⚠ EMERGENCY: Bin critically full + odour detected!");
      led.blink(Adafruit_NeoPixel::Color(255, 0, 0), 5, 100);
      publishPayload();
    }
  }

  // 6. OLED auto-dim check
  oled.dimCheck(now);

  delay(50);  // Small yield — keeps loop responsive without hammering PIR
}
