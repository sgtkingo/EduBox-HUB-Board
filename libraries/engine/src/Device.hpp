/**
 * @file Device.hpp
 * @brief Common hardware-device abstraction shared by all EduBox libraries.
 *
 * A device exposes one lifecycle and all three protocol-facing operations:
 * UPDATE reads runtime values, CONFIG changes internal configuration, and
 * CONTROL changes runtime output state. Unsupported operations use the default
 * no-op implementation.
 */

#pragma once

#include <Arduino.h>
#include <vector>

struct DeviceParameter {
  String key;
  String value;
};

struct DeviceValue {
  String key;
  String value;
};

enum class DeviceType : uint8_t {
  Unknown,
  Ds18b20,
  Dht11,
  DigitalInput,
  AnalogHall,
  UltrasonicHcSr04,
  Bmp280,
  Bmp180,
  ColorTcs34725,
  InfraredReceiver,
  AnalogNtc,
  Photoresistor,
  Joystick,
  LinearHall,
  SharpDistance,
  RotaryEncoder,
  SmallMicrophone,
  LargeMicrophone,
  Heartbeat,
  ServoSg90,
  StepperMotor,
  DcMotor,
  TwoColorLed,
  MiniTwoColorLed,
  RgbLed,
  SevenColorLed,
  InfraredTransmitter,
  Laser,
  PassiveBuzzer,
  ActiveBuzzer
};

inline const char* deviceTypeName(DeviceType type) {
  switch (type) {
    case DeviceType::Ds18b20: return "DS18B20";
    case DeviceType::Dht11: return "DHT11";
    case DeviceType::DigitalInput: return "DigitalInput";
    case DeviceType::AnalogHall: return "AnalogHall";
    case DeviceType::UltrasonicHcSr04: return "HC-SR04";
    case DeviceType::Bmp280: return "BMP280";
    case DeviceType::Bmp180: return "BMP180";
    case DeviceType::ColorTcs34725: return "TCS34725";
    case DeviceType::InfraredReceiver: return "InfraredReceiver";
    case DeviceType::AnalogNtc: return "AnalogNTC";
    case DeviceType::Photoresistor: return "Photoresistor";
    case DeviceType::Joystick: return "Joystick";
    case DeviceType::LinearHall: return "LinearHall";
    case DeviceType::SharpDistance: return "GP2Y0A21YK0F";
    case DeviceType::RotaryEncoder: return "RotaryEncoder";
    case DeviceType::SmallMicrophone: return "SmallMicrophone";
    case DeviceType::LargeMicrophone: return "LargeMicrophone";
    case DeviceType::Heartbeat: return "Heartbeat";
    case DeviceType::ServoSg90: return "SG90";
    case DeviceType::StepperMotor: return "StepperMotor";
    case DeviceType::DcMotor: return "DCMotor";
    case DeviceType::TwoColorLed: return "TwoColorLED";
    case DeviceType::MiniTwoColorLed: return "MiniTwoColorLED";
    case DeviceType::RgbLed: return "RGBLED";
    case DeviceType::SevenColorLed: return "SevenColorLED";
    case DeviceType::InfraredTransmitter: return "InfraredTransmitter";
    case DeviceType::Laser: return "Laser";
    case DeviceType::PassiveBuzzer: return "PassiveBuzzer";
    case DeviceType::ActiveBuzzer: return "ActiveBuzzer";
    case DeviceType::Unknown: return "Unknown";
  }
  return "Unknown";
}

class Device {
public:
  virtual ~Device() = default;

  virtual DeviceType deviceType() const = 0;
  virtual size_t requiredPinCount() const { return 1; }
  virtual bool init() { return true; }
  virtual std::vector<DeviceValue> update() { return {}; }
  virtual void config(DeviceParameter* parameters = nullptr, int count = 0) {
    (void)parameters;
    (void)count;
  }
  virtual void control(DeviceParameter* parameters = nullptr, int count = 0) {
    (void)parameters;
    (void)count;
  }
  virtual void reset() {}
  virtual void attach(const std::vector<int>& pins) { (void)pins; }
  virtual void detach() {}
};

using Param = DeviceParameter;
using KV = DeviceValue;

struct RegisteredDevice {
  RegisteredDevice(const char* deviceUid, Device* deviceInstance)
      : uid(deviceUid), device(deviceInstance) {}

  const char* uid;
  Device* device;
  bool connected = false;
  bool initialized = false;
  std::vector<int> pins;
};
