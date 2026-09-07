/**
 * @file Senzor_Joystick.cpp
 * @brief Implements joystick calibration and direction detection.
 */

#include <Senzor_Joystick.hpp>

void Joystick::calibrateCenter(int resolutionBits) {
  analogReadResolution(resolutionBits);
  maximumAdcValue_ = (1UL << resolutionBits) - 1;

#ifdef ESP32
  // ADC_11db covers the joystick's full 0-3.3 V output range.
  analogSetPinAttenuation(xPin_, ADC_11db);
  analogSetPinAttenuation(yPin_, ADC_11db);
#endif

  long xSampleSum = 0;
  long ySampleSum = 0;
  constexpr int SAMPLE_COUNT = 16;
  for (int sample = 0; sample < SAMPLE_COUNT; ++sample) {
    xSampleSum += analogRead(xPin_);
    ySampleSum += analogRead(yPin_);
    delay(2);
  }
  centerX_ = static_cast<int>(xSampleSum / SAMPLE_COUNT);
  centerY_ = static_cast<int>(ySampleSum / SAMPLE_COUNT);

  previousResolutionBits_ = resolutionBits;
  calibrated_ = true;
}

int Joystick::deadZoneTolerance(int percent) const {
  int boundedPercent = percent;
  if (boundedPercent < 0) boundedPercent = 0;
  if (boundedPercent > 100) boundedPercent = 100;
  return static_cast<int>((boundedPercent * static_cast<long>(maximumAdcValue_)) / 100L);
}

std::vector<KV> Joystick::update() {
  if (!pinsInitialized_) {
    if (switchPin_ >= 0) pinMode(switchPin_, INPUT_PULLUP);
    pinsInitialized_ = true;
  }

  // Recalibrate after reset or an ADC resolution change. The stick must rest at center.
  if (!calibrated_ || resolutionBits_ != previousResolutionBits_) {
    calibrateCenter(resolutionBits_);
  } else {
    analogReadResolution(resolutionBits_);
  }

  const int xValue = analogRead(xPin_);
  const int yValue = analogRead(yPin_);
  const bool clicked = switchPin_ >= 0 && digitalRead(switchPin_) == LOW;

  const int tolerance = deadZoneTolerance(deadZonePercent_);
  int xOffset = xValue - centerX_;
  int yOffset = yValue - centerY_;

  if (abs(xOffset) < tolerance) xOffset = 0;
  if (abs(yOffset) < tolerance) yOffset = 0;

  // Preserve the board's historical axis mapping when reporting direction.
  String direction;
  if (clicked) {
    direction = "CLICK";
  } else if (xOffset == 0 && yOffset == 0) {
    direction = "CENTER";
  } else if (abs(yOffset) >= abs(xOffset)) {
    direction = (yOffset < 0) ? "LEFT" : "RIGHT";
  } else {
    direction = (xOffset > 0) ? "UP" : "DOWN";
  }

  return {{"direction", direction}};
}
