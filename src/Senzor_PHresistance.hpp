/**
 * @file Senzor_PHresistance.hpp
 * @brief Photoresistor device declaration with ADC and gain configuration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"


class PHresistance : public Sensor {
public:
  // konstruktor, pin bude přiřazen pouze přes attach()
  explicit PHresistance(int pin = -1)
    : _pin(pin), _res(12), _gain(1.0f) {
  }

  bool            init()   override;                
  void            reset()  override {}
  std::vector<KV> update() override;                
  DeviceType deviceType() const override { return DeviceType::Photoresistor; }

  // Konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "res")  _res  = params[i].value.toInt();
      else if (k == "gain") _gain = params[i].value.toFloat();
    }
    analogReadResolution(_res);
  }

  // PIN se bere pouze z attach()
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, INPUT);
        analogReadResolution(_res);
      }
    }
  }

  // detach: bezpečné uvolnění pinu
  void detach() override {
    if (_pin >= 0) {
      pinMode(_pin, INPUT);
      _pin = -1;
    }
  }

private:
  int   _pin;
  int   _res;    // ADC bity
  float _gain;   // násobek na výstupu (kalibrace)
};
