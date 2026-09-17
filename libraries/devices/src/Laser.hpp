/**
 * @file Laser.hpp
 * @brief Laser output device declaration with safe attach and detach lifecycle.
 */

#pragma once
#include <Arduino.h>
#include <Actuator.hpp>

void Laser_setPin(int pin);
void Laser_control(bool control);
void Laser_reset();

class Laser : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::Laser; }

  Laser(int pin = -1, bool control = false)
    : _pin(pin), _control(control) {}


  //Přiřazení pinu 
  void attach(const std::vector<int>& pins) override {
    if (pins.size() >= 1) {
      _pin = pins[0];
      _control = false;
      Laser_setPin(_pin);
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, LOW);
    }
  }

  // bezpečně vypnout a uvolnit pin
  void detach() override {
    Laser_reset();
    if (_pin >= 0) {
      digitalWrite(_pin, LOW);
      pinMode(_pin, INPUT);
      _pin = -1;
      Laser_setPin(-1);
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
    Laser_control(_control);
  }

  void reset() override { Laser_reset(); }

private:
  int _pin;
  bool _control;
};
