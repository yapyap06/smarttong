/**
 * SmartTONG AI — Config.h
 * Central configuration for WiFi, Firebase, and pin assignments.
 * Change ONLY this file to adapt to your physical wiring.
 */

#pragma once

// ─── Demo / Competition Mode ────────────────────────────────────────────────
// Set DEMO_MODE 1 for the judging booth — keeps ESP32 always-on (no deep sleep).
// Set to 0 for real deployment to enable 30-min deep sleep cycles.
#define DEMO_MODE        1

// ─── WiFi ────────────────────────────────────────────────────────────────────
// For Wokwi simulation: use "Wokwi-GUEST" / ""
// For real hardware:   replace with your actual WiFi credentials
#define WIFI_SSID        "Wokwi-GUEST"
#define WIFI_PASSWORD    ""
#define WIFI_TIMEOUT_MS  15000

// ─── Firebase ────────────────────────────────────────────────────────────────
// Replace with your actual Firebase project values
#define FIREBASE_HOST    "smarttong-selangor-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH    ""   // Database secret not required for Test Mode
#define FIREBASE_DB_URL  "https://smarttong-selangor-default-rtdb.firebaseio.com"

// ─── Supabase (alternate backend) ────────────────────────────────────────────
// Leave blank to use Firebase only. Fill in to dual-publish to Supabase REST.
#define SUPABASE_URL     ""   // e.g. "https://xyz.supabase.co"
#define SUPABASE_KEY     ""   // anon/service-role key
#define SUPABASE_TABLE   "bin_readings"

// ─── Bin Identity ────────────────────────────────────────────────────────────
#define BIN_ID           "B01"
#define BIN_LOCATION_LAT  3.0738f
#define BIN_LOCATION_LNG  101.5183f

// ─── Publish interval ────────────────────────────────────────────────────────
#define PUBLISH_INTERVAL_MS   600000UL   // 10 minutes normal cadence
#define EVENT_PUBLISH_MS       5000UL    // 5 s after a lid-open event

// ─── Deep Sleep (Phase 2) ─────────────────────────────────────────────────────
// Only active when DEMO_MODE == 0
#define DEEP_SLEEP_INTERVAL_US  1800000000ULL  // 30 minutes in microseconds
#define WAKE_REASON_TIMER       0
#define WAKE_REASON_PIR         1

// ─── Ultrasonic Sensor (JSN-SR04T / HC-SR04) ─────────────────────────────────
#define ULTRASONIC_TRIG_PIN   5
#define ULTRASONIC_ECHO_PIN   18
#define BIN_EMPTY_CM          40.0f   // distance when bin is empty
#define BIN_FULL_CM           5.0f    // distance when bin is full (sensor to trash top)

// ─── MQ-135 Gas Sensor ───────────────────────────────────────────────────────
#define GAS_SENSOR_PIN        34      // ADC1 channel (use 32-39 on ESP32)
#define GAS_ALERT_THRESHOLD   300     // raw ADC value above which odour is flagged

// ─── PIR Motion Sensor ───────────────────────────────────────────────────────
#define PIR_PIN               14

// ─── Servo (SG90) ────────────────────────────────────────────────────────────
#define SERVO_PIN             13
#define SERVO_OPEN_DEG        90
#define SERVO_CLOSED_DEG      0
#define LID_OPEN_DURATION_MS  10000UL   // 10 seconds open time

// ─── NeoPixel LED ────────────────────────────────────────────────────────────
#define LED_PIN               27
#define LED_COUNT             1
#define LED_BRIGHTNESS        80        // 0–255

// ─── DFPlayer Mini (OKU Audio) ───────────────────────────────────────────────
// Connect DFPlayer RX → ESP32 TX2 (pin 17), DFPlayer TX → ESP32 RX2 (pin 16)
#define DFPLAYER_RX_PIN       16        // ESP32 RX (receives DFPlayer TX)
#define DFPLAYER_TX_PIN       17        // ESP32 TX (sends to DFPlayer RX)
#define DFPLAYER_VOLUME       25        // 0–30

// Audio track map (files must be named 0001.mp3, 0002.mp3, etc. on SD card)
#define AUDIO_BIN_OK          1   // "Terima kasih. Tong masih lega."
#define AUDIO_BIN_HALF        2   // "Tong hampir penuh. Sila gunakan tong berdekatan."
#define AUDIO_BIN_FULL        3   // "Tong penuh. Tong terdekat dalam 20 meter."
#define AUDIO_HAZARDOUS       4   // "Amaran: Bahan berbahaya dikesan."
#define AUDIO_THANK_YOU       5   // "Terima kasih kerana menjaga kebersihan Selangor."

// ─── Fill thresholds ─────────────────────────────────────────────────────────
#define FILL_AMBER_PERCENT    50
#define FILL_RED_PERCENT      80

// ─── Battery Monitor (Phase 1) ───────────────────────────────────────────────
// Voltage divider: R1 = 100kΩ (from VBAT), R2 = 47kΩ (to GND), ADC → GPIO35
// VBAT_measured = ADC_voltage × (R1 + R2) / R2
#define BATTERY_ADC_PIN       35       // ADC1 channel (do NOT use ADC2 with WiFi)
#define BATTERY_VDIV_R1       100000.0f  // 100 kΩ upper resistor
#define BATTERY_VDIV_R2        47000.0f  // 47 kΩ lower resistor
#define BATTERY_FULL_V          4.2f   // 18650 fully charged
#define BATTERY_EMPTY_V         3.0f   // 18650 safe discharge cutoff
#define BATTERY_ADC_REF_V       3.3f   // ESP32 ADC reference voltage
#define BATTERY_ADC_BITS        4095   // 12-bit ADC
#define BATTERY_LOW_PERCENT       15   // Alert threshold

// ─── OLED Display (Phase 1) ──────────────────────────────────────────────────
// SSD1306 0.96" 128×64 via I2C
#define OLED_SDA_PIN          21
#define OLED_SCL_PIN          22
#define OLED_I2C_ADDR         0x3C
#define OLED_SCREEN_W         128
#define OLED_SCREEN_H          64
#define OLED_DIM_TIMEOUT_MS   30000UL  // Auto-dim after 30 seconds

// ─── ESP32-CAM (when using separate CAM module) ───────────────────────────────
// Uncomment if using AI-Thinker ESP32-CAM
// #define USE_CAMERA
#define CAMERA_FRAMESIZE      FRAMESIZE_96X96
