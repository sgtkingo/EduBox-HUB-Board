/**
 * @file BoardConfig.hpp
 * @brief Board-level default pins and shared sensor-driver instances.
 *
 * Defaults are replaced by VSCP CONNECT assignments at runtime. They remain
 * useful for construction and bench testing before the first connection.
 */

#pragma once

#include "libs.hpp"

constexpr uint8_t joystickXPin = 15;
constexpr uint8_t joystickYPin = 7;
constexpr uint8_t joystickSwitchPin = 17;

constexpr uint8_t terminal1Pin = 15;  // ADC2_05
constexpr uint8_t terminal2Pin = 7;   // ADC2_04
constexpr uint8_t terminal3Pin = 4;   // ADC1_03
constexpr uint8_t terminal4Pin = 5;   // ADC1_04

constexpr uint8_t sensorSdaPin = 11;  // SDA_01
constexpr uint8_t sensorSclPin = 12;  // SCL_01

constexpr uint8_t microphoneSampleWindowMs = 50;

Adafruit_BMP280 bmp(&I2C);
Adafruit_BMP085 bmp180;
