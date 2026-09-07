/**
 * @file Senzor_HC_SR04.hpp
 * @brief HC-SR04 ultrasonic distance device declaration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"

class HCSR04 : public Sensor {
public:
  HCSR04(int trig, int echo)
    : _trig(trig), _echo(echo), _limit(150), _delayMs(40) {} // default: 150 cm, 40 ms


    void attach(const std::vector<int>& pins) override {
      if (pins.size() >= 1) _trig = pins[0];
      if (pins.size() >= 2) _echo = pins[1];
    }
    void detach() override {
      if (_trig >= 0) pinMode(_trig, INPUT);
      if (_echo >= 0) pinMode(_echo, INPUT);
    }

  bool            init()   override;                 
  void            reset()  override {}
  std::vector<KV> update() override;                
  DeviceType deviceType() const override { return DeviceType::UltrasonicHcSr04; }

  // Konfigurační parametry
  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "limit") _limit   = params[i].value.toInt(); // max cm
      else if (k == "delay") _delayMs = params[i].value.toInt(); // prodleva v ms mezi jednotlivými měřeními
    }
  }

private:
  int _trig, _echo;
  int _limit;     // cm
  int _delayMs;   // ms
};
