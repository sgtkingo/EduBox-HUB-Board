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

private:
  int _pin;
  int _time;   // délka měření v ms
};
