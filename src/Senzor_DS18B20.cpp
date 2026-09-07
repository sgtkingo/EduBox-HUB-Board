/**
 * @file Senzor_DS18B20.cpp
 * @brief Implementation of the dynamically attached DS18B20 device.
 */

#include "Senzor_DS18B20.hpp"

DS18B20::DS18B20() = default;

DS18B20::~DS18B20() {
  delete sensors_;
  delete oneWire_;
}

void DS18B20::attach(const std::vector<int>& pins) {
  if (pins.empty()) return;
  detach();

  busPin_ = pins[0];
  if (busPin_ < 0) return;
  pinMode(busPin_, INPUT_PULLUP);
  oneWire_ = new OneWire(busPin_);
  sensors_ = new DallasTemperature(oneWire_);
}

void DS18B20::detach() {
  delete sensors_;
  delete oneWire_;
  sensors_ = nullptr;
  oneWire_ = nullptr;
  if (busPin_ >= 0) pinMode(busPin_, INPUT);
  busPin_ = -1;
}

bool DS18B20::init() {
  if (!sensors_) return false;
  sensors_->begin();
  if (sensors_->getDeviceCount() == 0) return false;
  applyConfig();
  return true;
}

void DS18B20::reset() {
  if (sensors_) {
    sensors_->begin();
    applyConfig();
  }
}

void DS18B20::config(Param* parameters, int count) {
  for (int index = 0; index < count; ++index) {
    String key = parameters[index].key;
    key.trim();
    key.toLowerCase();
    if (key == "res") resolution_ = parameters[index].value.toInt();
    else if (key == "lalarm" || key == "lowalarm") lowAlarm_ = parameters[index].value.toInt();
    else if (key == "halarm" || key == "highalarm") highAlarm_ = parameters[index].value.toInt();
  }
  applyConfig();
}

void DS18B20::applyConfig() {
  if (!sensors_) return;
  DeviceAddress address;
  if (!sensors_->getAddress(address, 0)) return;

  if (resolution_ >= 9 && resolution_ <= 12) sensors_->setResolution(address, resolution_);
  if (lowAlarm_ >= -55 && lowAlarm_ <= 125) sensors_->setLowAlarmTemp(address, lowAlarm_);
  if (highAlarm_ >= -55 && highAlarm_ <= 125) sensors_->setHighAlarmTemp(address, highAlarm_);
  sensors_->saveScratchPad(address);
}

std::vector<KV> DS18B20::update() {
  std::vector<KV> values;
  if (!sensors_) return values;

  DeviceAddress address;
  if (!sensors_->getAddress(address, 0)) return values;
  sensors_->requestTemperatures();
  const float temperature = sensors_->getTempC(address);
  if (temperature <= -126.0f) return values;

  const int lowAlarm = sensors_->getLowAlarmTemp(address);
  const int highAlarm = sensors_->getHighAlarmTemp(address);
  String alarm = "OK";
  if (temperature <= lowAlarm) alarm = "LOW";
  else if (temperature >= highAlarm) alarm = "HIGH";

  values.push_back({"temp", String(temperature, 1)});
  values.push_back({"alarm", alarm});
  return values;
}
