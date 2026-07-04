/**
 * SmartTONG AI — CameraAI.cpp
 * Waste classifier — mock mode (Wokwi) + real Edge Impulse mode (hardware).
 */

#include "CameraAI.h"
#include "Config.h"

// ─── Edge Impulse Real Classifier ────────────────────────────────────────────
#ifdef USE_REAL_CLASSIFIER
  // After installing Edge Impulse library, uncomment and adjust the include:
  // #include <SmartTong_inferencing.h>
  // #include "esp_camera.h"

  // Camera pin config for AI-Thinker ESP32-CAM
  // #define CAM_PIN_PWDN     32
  // #define CAM_PIN_RESET    -1
  // #define CAM_PIN_XCLK      0
  // #define CAM_PIN_SIOD     26
  // #define CAM_PIN_SIOC     27
  // #define CAM_PIN_D7       35
  // #define CAM_PIN_D6       34
  // #define CAM_PIN_D5       39
  // #define CAM_PIN_D4       38
  // #define CAM_PIN_D3       37
  // #define CAM_PIN_D2       36
  // #define CAM_PIN_D1       21
  // #define CAM_PIN_D0       19
  // #define CAM_PIN_VSYNC    25
  // #define CAM_PIN_HREF     23
  // #define CAM_PIN_PCLK     22
#endif

// ─── Labels matching Edge Impulse training labels ────────────────────────────
static const char* WASTE_LABELS[] = {
  "organic",
  "recyclable",
  "hazardous",
  "empty",
  "unknown"
};

void CameraAI::begin() {
#ifdef USE_REAL_CLASSIFIER
  // Real camera init would go here
  Serial.println("[CameraAI] Real classifier mode — ESP32-CAM init.");
  // camera_config_t config = { ... };
  // esp_camera_init(&config);
#else
  Serial.println("[CameraAI] SIMULATION mode — mock classifier active.");
  Serial.println("[CameraAI] Replace mockClassify() with run_classifier() when Edge Impulse library is installed.");
  randomSeed(analogRead(0));
#endif
}

ClassificationResult CameraAI::classify() {
#ifdef USE_REAL_CLASSIFIER
  // Real inference:
  // camera_fb_t* fb = esp_camera_fb_get();
  // signal_t signal;
  // ... set up signal from fb->buf ...
  // ei_impulse_result_t result;
  // run_classifier(&signal, &result, false);
  // esp_camera_fb_return(fb);
  // WasteType best = WASTE_UNKNOWN;
  // float bestConf = 0;
  // for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
  //   if (result.classification[i].value > bestConf) {
  //     bestConf = result.classification[i].value;
  //     best = (WasteType)i;
  //   }
  // }
  // return { best, WASTE_LABELS[best], bestConf };
  return mockClassify(); // fallback until uncommented
#else
  return mockClassify();
#endif
}

ClassificationResult CameraAI::mockClassify() {
  // Cycle deterministically through classes for a convincing demo
  const WasteType sequence[] = {
    WASTE_ORGANIC, WASTE_RECYCLABLE, WASTE_ORGANIC,
    WASTE_RECYCLABLE, WASTE_HAZARDOUS, WASTE_ORGANIC,
    WASTE_EMPTY, WASTE_RECYCLABLE
  };
  const int SEQ_LEN = 8;

  WasteType t = sequence[_mockIndex % SEQ_LEN];
  _mockIndex++;

  float conf = 0.80f + (random(0, 15) / 100.0f);  // 0.80 – 0.95
  Serial.printf("[CameraAI] Mock classify → %s (%.2f)\n", WASTE_LABELS[t], conf);
  return { t, WASTE_LABELS[t], conf };
}

const char* CameraAI::wasteTypeToString(WasteType t) {
  if (t >= 0 && t <= 4) return WASTE_LABELS[t];
  return "unknown";
}
