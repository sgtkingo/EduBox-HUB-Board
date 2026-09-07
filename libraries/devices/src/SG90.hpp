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
    _servo.attach(_pin);
    writeSmooth_(0, 0);
    _lastAngle = 0;
    SG90_setPin(_pin); 
    return true;
  }

  // Reset = reinicializace
  void reset() override { init(); }

  
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      int newPin = pins[0];
      if (newPin != _pin) {
        if (_servo.attached()) _servo.detach();
        if (_pin >= 0) pinMode(_pin, INPUT);
        _pin = newPin;
        _servo.attach(_pin);
        SG90_setPin(_pin); 
      }
    }
  }

  // Bezpečné uvolnění při DISCONNECT
  void detach() override {
    if (_servo.attached()) _servo.detach();
    if (_pin >= 0) pinMode(_pin, INPUT);
    SG90_setPin(-1);
  }

private:
  // Převod speed(0..100) -> delay mezi kroky (ms), 0 = nejrychleji
  static int speedToDelayMs_(int speed) {
    speed = constrain(speed, 0, 100);
   
    return map(speed, 0, 100, 100, 0);
  }

  void writeSmooth_(int targetAngle, int speedMs) {
    targetAngle = constrain(targetAngle, 0, 180);
    if (speedMs <= 0) { _servo.write(targetAngle); return; }
    int step = (_lastAngle < targetAngle) ? 1 : -1;
    for (int pos = _lastAngle; pos != targetAngle; pos += step) {
      _servo.write(pos);
      delay(speedMs);
    }
    _servo.write(targetAngle);
  }

  void apply_() {
    if (_pin < 0) return;
    if (!_servo.attached()) _servo.attach(_pin);
    int d = speedToDelayMs_(_speed);
    _angle = constrain(_angle, 0, 180);
    if (_angle != _lastAngle) {
      writeSmooth_(_angle, d);
      _lastAngle = _angle;
    } else {
      _servo.write(_angle);
    }
  }

  Servo _servo;      
  int   _pin   = -1;
  int   _angle = 0;   // 0..180
  int   _speed = 0;   // 0..100
  int   _lastAngle = 0;
};
