/**
 * @file Senzor_Ahall.hpp
 * @brief Analog Hall-effect sensor declaration with configurable thresholds.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"

class Ahall : public Sensor {
public:
  // konstruktor, pin se bere pouze z attach
  explicit Ahall(int pin = -1)
    : _pin(pin), _res(12), _lLimit(1800), _hLimit(2000), _val(0) {}


  std::vector<KV> update() override;
  void reset() override { _val = 0; }
  bool init() override { return true; }
  DeviceType deviceType() const override { return DeviceType::AnalogHall; }

  // Konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "res")     _res    = params[i].value.toInt();
      else if (k == "llimit")  _lLimit = params[i].value.toInt();
      else if (k == "hlimit")  _hLimit = params[i].value.toInt();
    }
    // aplikace ADC rozlišení a nastavení vstupu pokud je pin přiřazen
    analogReadResolution(_res);
    if (_pin >= 0) pinMode(_pin, INPUT);
  }

  // přiřazení pinu přes attach, použije první pin
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, INPUT);
        analogReadResolution(_res);
      }
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
  int _pin;
  int _res;
  int _lLimit;
  int _hLimit;
  int _val;
};
