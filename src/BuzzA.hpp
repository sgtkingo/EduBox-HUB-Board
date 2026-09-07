/**
 * @file BuzzA.hpp
 * @brief Active-buzzer device declaration and binary control interface.
 */

#pragma once
#include <Arduino.h>
#include "actuator.hpp"


void BuzzA_control(int pin, bool control);
void BuzzA_reset(int pin);

class BuzzA : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::ActiveBuzzer; }

  BuzzA(int pin, bool control)
    : _pin(pin), _control(control) {}

  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      if (_pin >= 0 && _pin != pins[0]) {
        digitalWrite(_pin, LOW);
        pinMode(_pin, INPUT);
      }
      _pin = pins[0];
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, LOW);
    }
  }

  void detach() override {
  if (_pin >= 0) {
    digitalWrite(_pin, LOW);
    pinMode(_pin, INPUT);
  }
  }

  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
        String k = params[i].key;
        k.trim();
        k.toLowerCase();
        if (k == "control") {
          String v = params[i].value;
          v.trim();
          v.toLowerCase();
          _control = (v == "true" || v == "1" || v == "on");
      }
    }
    BuzzA_control(_pin, _control);
  }

  void reset() override { BuzzA_reset(_pin); }

private:
  int _pin;
  bool _control;
};
