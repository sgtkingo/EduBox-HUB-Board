/**
 * @file Senzor_Heartbeat.hpp
 * @brief Heartbeat sensor declaration with configurable measurement window.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include <Sensor.hpp>

class Heartbeat : public Sensor {
public:
  Heartbeat(int pin, int time_ms) : _pin(pin), _time(time_ms) {}

  bool            init()   override;                 // rychlý sanity check
  void            reset()  override{};
  std::vector<KV> update() override;                 // vrací {"bpm", ...}
  DeviceType deviceType() const override { return DeviceType::Heartbeat; }
  void attach(const std::vector<int>& pins) override {
    if (pins.size() != requiredPinCount()) {
      detach();
      return;
    }
    _pin = pins[0];
    pinMode(_pin, INPUT);
  }
  void detach() override {
    if (_pin >= 0) pinMode(_pin, INPUT);
    _pin = -1;
  }

private:
  int _pin;
  int _time;   // délka měření v ms
};
