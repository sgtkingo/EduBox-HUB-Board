/**
 * @file Senzor_Joystick.hpp
 * @brief Three-channel joystick device declaration and calibration state.
 */

#pragma once

#include <Arduino.h>
#include <vector>

#include "Sensor.hpp"

class Joystick : public Sensor {
public:
  Joystick(int xPin, int yPin, int switchPin)
      : xPin_(xPin), yPin_(yPin), switchPin_(switchPin),
        resolutionBits_(12), deadZonePercent_(25), calibrated_(false),
        centerX_(0), centerY_(0), previousResolutionBits_(-1),
        maximumAdcValue_(4095), pinsInitialized_(false) {
    analogReadResolution(resolutionBits_);
  }

  void attach(const std::vector<int>& pins) override {
    if (pins.size() >= 1) xPin_ = pins[0];
    if (pins.size() >= 2) yPin_ = pins[1];
    if (pins.size() >= 3) switchPin_ = pins[2];
    pinsInitialized_ = false;
    calibrated_ = false;
  }

  void detach() override {
    if (xPin_ >= 0) pinMode(xPin_, INPUT);
    if (yPin_ >= 0) pinMode(yPin_, INPUT);
    if (switchPin_ >= 0) pinMode(switchPin_, INPUT);
    pinsInitialized_ = false;
    calibrated_ = false;
  }

  void reset() override { calibrated_ = false; }
  std::vector<KV> update() override;
  DeviceType deviceType() const override { return DeviceType::Joystick; }
  bool init() override { return true; }

  /** Changes ADC resolution and joystick dead-zone configuration. */
  void config(Param* parameters = nullptr, int count = 0) override {
    for (int index = 0; index < count; ++index) {
      String key = parameters[index].key;
      key.trim();
      key.toLowerCase();
      if (key == "res") resolutionBits_ = parameters[index].value.toInt();
      else if (key == "threshold") deadZonePercent_ = parameters[index].value.toInt();
    }
    analogReadResolution(resolutionBits_);
  }

private:
  void calibrateCenter(int resolutionBits);
  int deadZoneTolerance(int percent) const;

  int xPin_;
  int yPin_;
  int switchPin_;
  int resolutionBits_;
  int deadZonePercent_;
  bool calibrated_;
  int centerX_;
  int centerY_;
  int previousResolutionBits_;
  uint32_t maximumAdcValue_;
  bool pinsInitialized_;
};
