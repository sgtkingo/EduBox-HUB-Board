/**
 * @file TwoColorMini.hpp
 * @brief Compact two-color LED device declaration.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include <Actuator.hpp>

void TwoColorMini_setPins(int pinRed, int pinGreen);
void TwoColorMini_control(char color, int Brightness);
void TwoColorMini_reset();

class TwoColorMini : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::MiniTwoColorLed; }
  size_t requiredPinCount() const override { return 2; }

  TwoColorMini(int pinRed = -1, int pinGreen = -1, char color = 'r', int Brightness = 0)
    : _pinRed(pinRed), _pinGreen(pinGreen), _color(color), _Brightness(Brightness) {}

  void attach(const std::vector<int>& pins) override {
    if (pins.size() != requiredPinCount()) {
      detach();
      return;
    }
    _pinRed = pins[0];
    _pinGreen = pins[1];
    TwoColorMini_setPins(_pinRed, _pinGreen);
  }

  void detach() override {
    TwoColorMini_reset();
    if (_pinRed >= 0) { digitalWrite(_pinRed, LOW); pinMode(_pinRed, INPUT); _pinRed = -1; }
    if (_pinGreen >= 0) { digitalWrite(_pinGreen, LOW); pinMode(_pinGreen, INPUT); _pinGreen = -1; }
  }

  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "color") _color = params[i].value.charAt(0);
      else if (k == "brig") _Brightness = params[i].value.toInt();
    }
    TwoColorMini_control(_color, _Brightness);
  }

  void reset() override { TwoColorMini_reset(); }

private:
  int _pinRed   = -1;
  int _pinGreen = -1;
  char _color   = 'r';
  int  _Brightness = 0;
};
