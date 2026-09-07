/**
 * @file Stepper.hpp
 * @brief Four-wire stepper motor device declaration and driver lifecycle.
 */

#pragma once
#include <Arduino.h>
#include "actuator.hpp"

class CheapStepper;

class Stepper : public Actuator {
public:
  Stepper(int pin1, int pin2, int pin3, int pin4, int angle, bool dir, int rpm)
    : _pin1(pin1), _pin2(pin2), _pin3(pin3), _pin4(pin4),
      _angle(angle), _dir(dir), _rpm(rpm) {}



  void attach(const std::vector<int>& pins) override;
  void detach() override;
  DeviceType deviceType() const override { return DeviceType::StepperMotor; }
  bool init() override;
  void control(Param* params = nullptr, int count = 0) override;
  void reset() override;

private:
  int _pin1 = -1, _pin2 = -1, _pin3 = -1, _pin4 = -1;
  CheapStepper* _stp = nullptr;

  int  _angle = 0;     // úhel v °
  bool _dir   = true;  // true=fwd, false=back
  int  _rpm   = 16;    // RPM (revolutions per minute)
  int  _currentPos = 0; // akumulace v ° (pro reset)

  void ensureDriver_();       // vytvoří _stp, nastaví piny
  void releasePins_();        // piny do INPUT
  //void coilsOff_();           // vypnout vinutí (šetří proud)
};
