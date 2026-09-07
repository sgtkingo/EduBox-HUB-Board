/**
 * @file Senzor_DS18B20.hpp
 * @brief DS18B20 temperature device with a dynamically assigned OneWire bus.
 */

#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <vector>

#include "Sensor.hpp"

class DS18B20 : public Sensor {
public:
  DS18B20();
  ~DS18B20() override;

  DeviceType deviceType() const override { return DeviceType::Ds18b20; }
  bool init() override;
  void reset() override;
  std::vector<KV> update() override;
  void config(Param* parameters = nullptr, int count = 0) override;
  void attach(const std::vector<int>& pins) override;
  void detach() override;

private:
  void applyConfig();

  OneWire* oneWire_ = nullptr;
  DallasTemperature* sensors_ = nullptr;
  int resolution_ = 12;
  int lowAlarm_ = -55;
  int highAlarm_ = 125;
  int busPin_ = -1;
};
