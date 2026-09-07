/**
 * @file Senzor_DigitalRead.hpp
 * @brief Generic digital-input device used by multiple EduBox modules.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"

class SensorDigitalRead : public Sensor {
public:
  // Konstruktor, pin lze předat pro test, produkčně se bere přes attach
  SensorDigitalRead(int pin = -1, int id = 0, const char* type = "digital")
    : _pin(pin), _id(id), _type(type), _attached(pin >= 0) {
    if (_attached && _pin >= 0) pinMode(_pin, INPUT);
  }

  bool init() override { return true; }
  void reset() override {}
  std::vector<KV> update() override;
  DeviceType deviceType() const override { return DeviceType::DigitalInput; }

  // PIN se bere pouze z attach()
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      _attached = (_pin >= 0);
      if (_attached) pinMode(_pin, INPUT);
    }
  }

  // bezpečné uvolnění pinu
  void detach() override {
    if (_attached && _pin >= 0) {
      pinMode(_pin, INPUT);
      _pin = -1;
      _attached = false;
    }
  }

private:
  int         _pin;    
  int         _id;    
  const char* _type;
  bool        _attached = false;
};
