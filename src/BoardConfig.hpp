/**
 * @file BoardConfig.hpp
 * @brief Board-level device defaults and application transport configuration.
 *
 * Defaults are replaced by VSCP CONNECT assignments at runtime. They remain
 * useful for construction and bench testing before the first connection.
 * Physical serial settings belong to the firmware application rather than the
 * transport-independent VSCP library.
 */

#pragma once

#include <Arduino.h>

struct UartTransportConfig {
  uint8_t port;
  uint32_t baudRate;
  uint32_t frameFormat;
  int8_t rxPin;
  int8_t txPin;
};

constexpr uint32_t usbProtocolBaudRate = 115200;
// Board-side control lease: valid commands or replies to Board PING renew it.
constexpr uint32_t vscpControlLeaseMs = 10000;
constexpr uint32_t vscpControlProbeIntervalMs = 3000;
constexpr uint32_t vscpControlProbeTimeoutMs = 500;
// UART0 / USB-UART diagnostics (not native USB CDC).
constexpr bool uartDebugEnabled = true;
constexpr bool uartDebugTraceEnabled = true;
constexpr bool usbProtocolEnabled = true;  // Keep emulator access on UART0
constexpr UartTransportConfig vscpUartConfig = {
  2,
  115200,
  SERIAL_8N1,
  18,
  17
};

constexpr uint8_t joystickXPin = 15;
constexpr uint8_t joystickYPin = 7;
constexpr uint8_t joystickSwitchPin = 16;  // Separated from UART2 TX pin 17

constexpr uint8_t terminal1Pin = 15;  // ADC2_05
constexpr uint8_t terminal2Pin = 7;   // ADC2_04
constexpr uint8_t terminal3Pin = 4;   // ADC1_03
constexpr uint8_t terminal4Pin = 5;   // ADC1_04

constexpr uint8_t sensorSdaPin = 11;  // SDA_01
constexpr uint8_t sensorSclPin = 12;  // SCL_01

constexpr uint8_t microphoneSampleWindowMs = 50;
