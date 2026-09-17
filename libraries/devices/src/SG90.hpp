/**
 * @file SG90.hpp
 * @brief SG90 servo device declaration with smooth angle control.
 */

#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include <Actuator.hpp>


void SG90_setPin(int pin);           
void SG90_control(int angle, int speed); 

class SG90 : public Actuator {
public:
  DeviceType deviceType() const override { return DeviceType::ServoSg90; }

  SG90(int pin, int angle, int speed)
    : _pin(pin), _angle(angle), _speed(speed) {}

  
  void control(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "angle") _angle = params[i].value.toInt();
      else if (k == "speed") _speed = params[i].value.toInt();
    }
  
    apply_();
  }

  // HW init až PO attach()
  bool init() override {
    if (_pin < 0) return false;
    if (_servo.attached()) _servo.detach();
    digitalWrite(_pin, LOW);
    pinMode(_pin, OUTPUT); // No PWM until an explicit CONTROL.
    _lastAngle = 0;
    _moving = false;
    SG90_setPin(_pin); 
    return true;
  }

  // Reset = reinicializace
  void reset() override {
    if (init()) { _angle = 0; apply_(); } // Explicit RESET retains the return-to-zero behavior.
  }

  
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      int newPin = pins[0];
      if (newPin != _pin) {
        if (_servo.attached()) _servo.detach();
        if (_pin >= 0) pinMode(_pin, INPUT);
        _pin = newPin;
        _angle = 0;
        SG90_setPin(_pin); 
      }
    }
  }

  // Bezpečné uvolnění při DISCONNECT
  void detach() override {
    _moving = false;
    if (_servo.attached()) _servo.detach();
    if (_pin >= 0) pinMode(_pin, INPUT);
    _pin = -1;
    SG90_setPin(-1);
  }

  void service() override {
    if (!_moving || !_servo.attached() || _pin < 0) return;
    const uint32_t now = millis();
    const uint32_t delayMs = static_cast<uint32_t>(speedToDelayMs_(_speed));
    if (static_cast<uint32_t>(now - _lastStepMs) < delayMs) return;
    _lastStepMs = now;
    _lastAngle += _lastAngle < _angle ? 1 : -1;
    _servo.write(_lastAngle);
    _moving = _lastAngle != _angle;
  }

private:
  // Převod speed(0..100) -> delay mezi kroky (ms), 0 = nejrychleji
  static int speedToDelayMs_(int speed) {
    speed = constrain(speed, 0, 100);
   
    return map(speed, 0, 100, 100, 0);
  }

  void apply_() {
    if (_pin < 0) return;
    if (!_servo.attached()) _servo.attach(_pin);
    int d = speedToDelayMs_(_speed);
    _angle = constrain(_angle, 0, 180);
    _lastStepMs = millis();
    _moving = d > 0 && _angle != _lastAngle;
    if (!_moving) {
      _servo.write(_angle);
      _lastAngle = _angle;
    } else {
      _servo.write(_lastAngle);
    }
  }

  Servo _servo;      
  int   _pin   = -1;
  int   _angle = 0;   // 0..180
  int   _speed = 0;   // 0..100
  int   _lastAngle = 0;
  uint32_t _lastStepMs = 0;
  bool _moving = false;
};
