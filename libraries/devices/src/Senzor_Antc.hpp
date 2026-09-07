/**
 * @file Senzor_Antc.hpp
 * @brief Analog NTC temperature sensor declaration and filter configuration.
 */

#pragma once

#include <Arduino.h>
#include <Sensor.hpp>
#include <vector>

class Antc : public Sensor {
public:
  enum class Filter { Off=0, Light=1, Heavy=2 };

  // konstruktor, pin se bere pouze z attach
  explicit Antc(int pin = -1)
    : _pin(pin), _res(12), _filter(Filter::Off) {
  }

  // čtení senzoru vrací páry k:v
  std::vector<KV> update() override;

  // reset interního stavu
  void reset() override {}

  // jednoduchá inicializace
  bool init() override { return true; }

  // typ senzoru
  DeviceType deviceType() const override { return DeviceType::AnalogNtc; }

  // Konfigurační parametry
  void config(Param* params=nullptr, int count=0) override {
    for (int i=0; i<count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "res") _res = params[i].value.toInt();
      else if (k == "filter") _filter = static_cast<Filter>(params[i].value.toInt());
    }
    
    // aplikace ADC rozlišení pokud je pin přiřazen
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
  // pomocná funkce pro přepočet teploty z ADC
  static float computeTemp(int adc, int resBits);

  // počet vzorků dle filtru
  int samplesFor(Filter f) const {
    return (f == Filter::Heavy) ? 10 : (f == Filter::Light ? 3 : 1);
  }

  int _pin;         // přiřazený pin nebo -1
  int _res;         // ADC rozlišení v bitech
  Filter _filter;   // filtr vzorků
};
