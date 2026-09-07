/**
 * @file Senzor_DHT11.hpp
 * @brief DHT11 temperature and humidity device declaration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include <DHT.h>
#include "Sensor.hpp"

// Třída pro DHT11 senzor, pin se přiřazuje pouze přes attach
class DHT11x : public Sensor {
public:
  // konstruktor, pin se bere pouze z attach
  explicit DHT11x(int pin = -1)
    : _pin(pin), _unitF(false), _useHI(false), _dht(nullptr) {}

  
  std::vector<KV> update() override;
  void reset() override;
  bool init() override;
  DeviceType deviceType() const override { return DeviceType::Dht11; }

  // Konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "unit") _unitF = (params[i].value == "F");
      else if (k == "hi")   _useHI = (params[i].value == "true");
    }
  }

  // PIN se bere pouze z attach, vytvoří se instance DHT a spustí begin
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, INPUT_PULLUP);
        if (_dht) { delete _dht; _dht = nullptr; }
        _dht = new DHT(_pin, DHT11);
        _dht->begin();
      }
    }
  }

  // bezpečné uvolnění pinu a zničení dosavadní instance DHT
  void detach() override {
    if (_dht) {
      delete _dht;
      _dht = nullptr;
    }
    if (_pin >= 0) {
      pinMode(_pin, INPUT);
      _pin = -1;
    }
  }

private:
  int  _pin;    
  bool _unitF;  // false = °C, true = °F
  bool _useHI;  // použití Heat Index
  DHT* _dht;    
};
