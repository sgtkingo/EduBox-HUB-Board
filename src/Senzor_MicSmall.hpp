/**
 * @file Senzor_MicSmall.hpp
 * @brief Small microphone module declaration and sample-window configuration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"


class MicSmall : public Sensor {
public:
  // konstruktor, pin lze předat nebo se nastaví přes attach
  MicSmall(int pin, int time_ms)
    : _pin(pin), _time(time_ms), _res(12) {
    if (_pin >= 0) pinMode(_pin, INPUT);
    analogReadResolution(_res);
  }

  bool init()   override;                
  void reset()  override {}
  std::vector<KV> update() override;       
  DeviceType deviceType() const override { return DeviceType::SmallMicrophone; }

  // Konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "res") _res  = params[i].value.toInt();
      else if (k == "time")  _time = params[i].value.toInt();   // ms
    }
    analogReadResolution(_res);
  }

  // přiřazení pinu přes attach, použije první položku
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, INPUT);
        analogReadResolution(_res);
      }
    }
  }

  // uvolnění pinu do high-impedance
  void detach() override {
    if (_pin >= 0) {
      pinMode(_pin, INPUT);
      _pin = -1;
    }
  }

private:
  int _pin;
  int _time;   // délka měření v ms
  int _res;    // ADC rozlišení v bitech

  int REF_BASE;   // referenční hladina
};
