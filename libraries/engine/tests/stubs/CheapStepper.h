#pragma once
class CheapStepper {
  int remaining_ = 0;
public:
  CheapStepper(int, int, int, int) {}
  void setRpm(int) {}
  void stop() { remaining_ = 0; }
  void moveDegrees(bool, int) {}
  void newMoveDegrees(bool, int angle) { remaining_ = angle; }
  void newMove(bool, int steps) { remaining_ = steps; }
  void run() { if (remaining_) --remaining_; }
  int getStepsLeft() const { return remaining_; }
};
