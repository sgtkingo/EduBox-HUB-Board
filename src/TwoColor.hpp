/**
 * @file TwoColor.hpp
 * @brief Two-color LED device declaration with color and brightness control.
 */

#pragma once
#include <Arduino.h>
#include "actuator.hpp"  

void TwoColor_control(int pinRed, int pinGreen, char color, int Brightness);
void TwoColor_reset();

class TwoColor : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::TwoColorLed; }

  TwoColor(int pinRed, int pinGreen, char color, int Brightness)
    : _pinRed(pinRed), _pinGreen(pinGreen), _color(color), _Brightness(Brightness) {}

  // připojení pinů 
  void attach(const std::vector<int>& pins) override {
    if (pins.size() >= 1) _pinRed   = pins[0];
    if (pins.size() >= 2) _pinGreen = pins[1];
  }

  // konfigurační parametry
  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "color") _color = params[i].value.charAt(0);
      else if (k == "brig")  _Brightness = params[i].value.toInt();
    }
    TwoColor_control(_pinRed, _pinGreen, _color, _Brightness);
  }

  void reset() override { TwoColor_reset(); }

  // bezpečné uvolnění pinů
  void detach() override {
    TwoColor_reset(); // nejdříve reset

    if (_pinRed >= 0) {
      digitalWrite(_pinRed, LOW);
      pinMode(_pinRed, INPUT);
      _pinRed = -1;
    }
    if (_pinGreen >= 0) {
      digitalWrite(_pinGreen, LOW);
      pinMode(_pinGreen, INPUT);
      _pinGreen = -1;
    }
  }

private:
  int  _pinRed   = -1;
  int  _pinGreen = -1;
  char _color    = 'r';
  int  _Brightness = 0;
};
