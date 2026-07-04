/**
 * SmartTONG AI — OKUAudio.cpp
 */

#include "OKUAudio.h"

// ─── DFPlayer Mini raw serial commands ───────────────────────────────────────
// Protocol: 0x7E FF 06 CMD FEEDBACK DAT1 DAT2 CHECKSUM EF
static void dfSendCommand(HardwareSerial& ser, uint8_t cmd, uint8_t dat1, uint8_t dat2) {
  uint16_t checksum = -(0xFF + 0x06 + cmd + 0x00 + dat1 + dat2);
  uint8_t buf[10] = {
    0x7E, 0xFF, 0x06, cmd, 0x00,
    dat1, dat2,
    (uint8_t)(checksum >> 8), (uint8_t)(checksum & 0xFF),
    0xEF
  };
  ser.write(buf, 10);
}

void OKUAudio::begin() {
  _dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(1000);   // DFPlayer needs ~1s after power-on

  // Check if serial is available (will be true on real hardware, may not respond in Wokwi)
  // Try to set volume — if no response it's simulation mode
  dfSendCommand(_dfSerial, 0x06, 0x00, DFPLAYER_VOLUME);  // Set volume
  delay(200);

  Serial.printf("[OKUAudio] Initialized on UART2 (RX=%d TX=%d), volume=%d\n",
                DFPLAYER_RX_PIN, DFPLAYER_TX_PIN, DFPLAYER_VOLUME);
  Serial.println("[OKUAudio] In Wokwi: audio track numbers will print here instead of playing.");
}

void OKUAudio::playForFillLevel(float fillPercent, bool hazardous) {
  if (hazardous) {
    Serial.println("[OKUAudio] Playing: HAZARDOUS WARNING");
    playTrack(AUDIO_HAZARDOUS);
    return;
  }
  if (fillPercent < FILL_AMBER_PERCENT) {
    Serial.println("[OKUAudio] Playing: BIN OK — Tong masih lega");
    playTrack(AUDIO_BIN_OK);
  } else if (fillPercent < FILL_RED_PERCENT) {
    Serial.println("[OKUAudio] Playing: BIN HALF — Tong hampir penuh");
    playTrack(AUDIO_BIN_HALF);
  } else {
    Serial.println("[OKUAudio] Playing: BIN FULL — Tong penuh");
    playTrack(AUDIO_BIN_FULL);
  }
}

void OKUAudio::playThankYou() {
  Serial.println("[OKUAudio] Playing: THANK YOU — Terima kasih");
  playTrack(AUDIO_THANK_YOU);
}

void OKUAudio::setVolume(int vol) {
  if (vol < 0) vol = 0;
  if (vol > 30) vol = 30;
  dfSendCommand(_dfSerial, 0x06, 0x00, (uint8_t)vol);
}

void OKUAudio::playTrack(int track) {
  // DFPlayer command 0x03 = play file by index
  dfSendCommand(_dfSerial, 0x03, 0x00, (uint8_t)track);
  delay(100);
}
