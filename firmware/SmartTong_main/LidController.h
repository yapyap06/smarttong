/**
 * SmartTONG AI — LidController.h
 * PIR-triggered servo lid: open on motion, auto-close after timeout.
 */

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Config.h"

class LidController {
public:
  void begin();
  void update();          // call every loop() tick
  bool isOpen() const;
  void forceOpen();
  void forceClose();

private:
  Servo   _servo;
  bool    _isOpen       = false;
  unsigned long _openedAt = 0;
};
