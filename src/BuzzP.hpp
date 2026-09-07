/**
 * @file BuzzP.hpp
 * @brief Passive-buzzer device declaration with frequency and duration control.
 */

#pragma once
#include <Arduino.h>
#include "actuator.hpp"


void BuzzP_control(int pin, int freq, int duration);
void BuzzP_reset(int pin);

class BuzzP : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::PassiveBuzzer; }

  BuzzP(int pin, int freq, int duration)
    : _pin(pin), _freq(freq), _duration(duration) {}

  void attach(const std::vector<int>& pins) override {
  if (!pins.empty()) {
    if (_pin >= 0 && _pin != pins[0]) noTone(_pin);
    _pin = pins[0];
    }
  }

  void detach() override {
  if (_pin >= 0) {
    noTone(_pin);
    pinMode(_pin, INPUT);
  }
  }

  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "freq") _freq = params[i].value.toInt();
      else if (k == "duration") _duration = params[i].value.toInt();
    }
    BuzzP_control(_pin, _freq, _duration);
  }

  void reset() override { BuzzP_reset(_pin); }
  bool init() override;

private:
  int _pin;
  int _freq;
  int _duration;
};
