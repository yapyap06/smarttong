/**
 * SmartTONG AI — CameraAI.h
 * Waste type classifier.
 *
 * In SIMULATION mode (default, works in Wokwi):
 *   classifyWaste() returns a randomly cycled mock result.
 *
 * In HARDWARE mode (when Edge Impulse library is installed):
 *   Uncomment #define USE_REAL_CLASSIFIER and include the
 *   Edge Impulse Arduino library. The rest of the codebase is unchanged.
 */

#pragma once
#include <Arduino.h>

// ── Uncomment the line below ONLY after installing Edge Impulse library ──────
// #define USE_REAL_CLASSIFIER

enum WasteType {
  WASTE_ORGANIC     = 0,
  WASTE_RECYCLABLE  = 1,
  WASTE_HAZARDOUS   = 2,
  WASTE_EMPTY       = 3,
  WASTE_UNKNOWN     = 4
};

struct ClassificationResult {
  WasteType type;
  const char* label;   // human-readable label
  float confidence;    // 0.0 – 1.0
};

class CameraAI {
public:
  void begin();
  ClassificationResult classify();
  const char* wasteTypeToString(WasteType t);

private:
  ClassificationResult mockClassify();

  // Cycle through classes deterministically for demo
  int _mockIndex = 0;
};
