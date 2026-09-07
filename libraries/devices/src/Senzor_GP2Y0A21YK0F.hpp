/**
 * @file Senzor_GP2Y0A21YK0F.hpp
 * @brief Sharp GP2Y0A21YK0F distance device declaration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include <Sensor.hpp>


class GP2Y0A21YK0F : public Sensor {
public:
  explicit GP2Y0A21YK0F(int pin)
    : _pin(pin), _unit(0), _lAlarm(22.0f), _hAlarm(78.0f) {}

  
  bool init() override;
  void reset() override {}                              
  std::vector<KV> update() override;                    
  DeviceType deviceType() const override { return DeviceType::SharpDistance; }

  // Config v HPP: jen uloží parametry (0=cm, 1=mm; limity ve stejné jednotce)
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "unit")     _unit   = params[i].value.toInt();   // 0=cm, 1=mm
      else if (k == "lowalarm") _lAlarm = params[i].value.toFloat();
      else if (k == "highalarm")_hAlarm = params[i].value.toFloat();
    }
  }
  // připojení pinu přes attach()
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) pinMode(_pin, INPUT);
    }
  }

  // uvolnění pinu 
  void detach() override {
    if (_pin >= 0) {
      pinMode(_pin, INPUT);
      _pin = -1;
    }
  }

private:
  int   _pin;
  int   _unit;     // 0 = cm, 1 = mm
  float _lAlarm;   // ve stejné jednotce jako _unit
  float _hAlarm;   // ve stejné jednotce jako _unit
};
