/**
 * @file Senzor_HallLin.hpp
 * @brief Linear Hall-effect sensor declaration with selectable output units.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include <Sensor.hpp>


class HallLin : public Sensor {
public:
  // konstruktor, pin se bere pouze z attach
  explicit HallLin(int pin = -1)
    : _pin(pin), _res(12), _unit("Voltage") {
      analogReadResolution(_res);
    }

  std::vector<KV> update() override;
  void            reset() override {}
  bool            init()  override;                 
  DeviceType deviceType() const override { return DeviceType::LinearHall; }

  // konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "res")  _res  = params[i].value.toInt();
      else if (k == "unit") _unit = params[i].value;
    }
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
  int    _pin;
  int    _res;
  String _unit;   // "ADC" | "Voltage" | "Induction"
};
