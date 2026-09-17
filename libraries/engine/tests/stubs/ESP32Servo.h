#pragma once
#include <Arduino.h>
class Servo {
  bool active_ = false;
public:
  bool attached() const { return active_; }
  void attach(int) { active_ = true; ++servoStarts; }
  void detach() { active_ = false; }
  void write(int) { ++servoWrites; }
};
