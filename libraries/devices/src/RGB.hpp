/**
 * @file RGB.hpp
 * @brief RGB LED device declaration with independent channel brightness control.
 */

#pragma once
#include <Arduino.h>
#include <Actuator.hpp>

void RGB_setPins(int pinR, int pinG, int pinB);
void RGB_control(int BrigR, int BrigG, int BrigB);
void RGB_reset();

class RGB : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::RgbLed; }
  size_t requiredPinCount() const override { return 3; }

  RGB(int pinR = -1, int pinG = -1, int pinB = -1, int BrigR = 0, int BrigG = 0, int BrigB = 0)
    : _pinR(pinR), _pinG(pinG), _pinB(pinB),
      _BrigR(BrigR), _BrigG(BrigG), _BrigB(BrigB) {}

  void attach(const std::vector<int>& pins) override {
    if (pins.size() != requiredPinCount()) {
      detach();
      return;
    }
    _pinR = pins[0];
    _pinG = pins[1];
    _pinB = pins[2];
    RGB_setPins(_pinR, _pinG, _pinB);
    _BrigR = _BrigG = _BrigB = 0;
    RGB_reset();
  }

  void detach() override {
    RGB_reset();
    if (_pinR >= 0) { digitalWrite(_pinR, LOW); pinMode(_pinR, INPUT); _pinR = -1; }
    if (_pinG >= 0) { digitalWrite(_pinG, LOW); pinMode(_pinG, INPUT); _pinG = -1; }
    if (_pinB >= 0) { digitalWrite(_pinB, LOW); pinMode(_pinB, INPUT); _pinB = -1; }
    RGB_setPins(-1, -1, -1);
  }

  // konfigurační parametry 
  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "brigr") _BrigR = params[i].value.toInt();
      else if (k == "brigg") _BrigG = params[i].value.toInt();
      else if (k == "brigb") _BrigB = params[i].value.toInt();
    }
    RGB_control(_BrigR, _BrigG, _BrigB);
  }

  void reset() override { RGB_reset(); }

private:
  int _pinR = -1;
  int _pinG = -1;
  int _pinB = -1;
  int _BrigR = 0;
  int _BrigG = 0;
  int _BrigB = 0;
};
