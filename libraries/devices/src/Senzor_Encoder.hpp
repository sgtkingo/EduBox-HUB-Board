/**
 * @file Senzor_Encoder.hpp
 * @brief Rotary encoder device declaration with direction and alarm limits.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include <Sensor.hpp>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/portmacro.h"

// Kvadraturní rotační enkodér (ISR-based) - kompatibilní s ESP32
class Rencoder : public Sensor {
public:
  Rencoder(int pinA = -1, int pinB = -1)
    : _pinA(pinA), _pinB(pinB),
      _lLimit(-100), _hLimit(100),
      _dir(1),
      _unit(-1), _last(0),
      _ticks(0), _prevState(0), _position(0) {}

  bool init() override;
  void reset() override;
  std::vector<KV> update() override;
  DeviceType deviceType() const override { return DeviceType::RotaryEncoder; }
  size_t requiredPinCount() const override { return 2; }

  void attach(const std::vector<int>& pins) override {
    if (pins.size() != requiredPinCount()) {
      detach();
      return;
    }
    _pinB = pins[0];
    _pinA = pins[1];
  }

  void detach() override;

  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if      (k == "direction") _dir = (params[i].value == "reverse") ? -1 : 1;
      else if (k == "lowalarm") _lLimit = params[i].value.toInt();
      else if (k == "highalarm") _hLimit = params[i].value.toInt();
    }
  }

private:
  int  _pinA, _pinB;
  int  _lLimit, _hLimit;
  int  _dir;
  int      _unit;
  int16_t  _last;

  // ISR-based counting
  volatile int64_t _ticks;     // akumulátor
  volatile uint8_t _prevState; // poslední AB stav
  long _position;              // poslední reportovaná pozice

  // ISR / synchronizace
  static portMUX_TYPE s_mux;
  static bool s_isrInstalled;
  static void IRAM_ATTR gpio_isr_router(void* arg);
  void IRAM_ATTR handleEdgeISR();

  bool startISR();
  void stopISR();
};
