/**
 * @file DC.hpp
 * @brief DC motor device declaration with speed and enabled-state control.
 */

#pragma once
#include <Arduino.h>
#include "actuator.hpp"
#include <vector>

void DC_control(int pin, int Speed, bool state);
void DC_reset();

class DC : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::DcMotor; }

  DC(int pin, int Speed, bool state)
    : _pin(pin), _Speed(Speed), _state(state) {}

  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "speed") {
        _Speed = params[i].value.toInt();
      } else if (k == "state") {
        String v = params[i].value;
        v.trim();
        v.toLowerCase();
        _state = (v == "true" || v == "1" || v == "on");
      }
    }
    DC_control(_pin, _Speed, _state);
  }

  void reset() override { DC_reset(); }

  // přiřazení pinu přes attach, použije první pin a aplikuje konfiguraci
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, OUTPUT);
        DC_control(_pin, _Speed, _state);
      }
    }
  }

  // uvolnění pinu a zastavení motoru
  void detach() override {
    if (_pin >= 0) {
      DC_reset();
      pinMode(_pin, INPUT);
      _pin = -1;
    }
  }

private:
  int _pin;
  int _Speed;
  bool _state;
};
