/**
 * SmartTONG AI — OKUAudio.h
 * DFPlayer Mini serial driver for OKU (visual impairment) audio feedback.
 *
 * Wokwi simulation: Audio commands are printed to Serial monitor
 * (DFPlayer Mini not available in Wokwi — use Serial output to verify logic).
 *
 * Hardware: Connect DFPlayer Mini RX → ESP32 TX2 (pin 17)
 *           DFPlayer Mini TX → ESP32 RX2 (pin 16)
 *           Load MP3 files 0001.mp3–0005.mp3 on microSD card.
 */

#pragma once
#include <Arduino.h>
#include "Config.h"

// Uncomment when DFPlayerMini_Fast library is installed:
// #include <DFPlayerMini_Fast.h>

class OKUAudio {
public:
  void begin();
  void playForFillLevel(float fillPercent, bool hazardous);
  void playThankYou();
  void setVolume(int vol);   // 0–30

private:
  void playTrack(int track);

  // Uncomment for real hardware:
  // DFPlayerMini_Fast _player;
  HardwareSerial _dfSerial{2};   // UART2: RX=16, TX=17
  bool _hwReady = false;
};
