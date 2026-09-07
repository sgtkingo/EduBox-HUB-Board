/**
 * @file Senzor_IRrx.hpp
 * @brief Asynchronous infrared receiver device declaration.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"


class IRrecv;
struct decode_results;

class IRrx : public Sensor {
public:
  explicit IRrx(int pin) : _pin(pin), _dedupMs(150) {}

  bool            init()   override;
  void            reset()  override;
  std::vector<KV> update() override;
  DeviceType deviceType() const override { return DeviceType::InfraredReceiver; }

  void config(Param* params = nullptr, int count = 0) override {
    for (int i = 0; i < count; ++i) {
      String k = params[i].key;
      k.trim();
      k.toLowerCase();
      if (k == "dedup") {
        _dedupMs = (unsigned long) strtoul(params[i].value.c_str(), nullptr, 10);
      }
    }
  }

  // přiřazení pinu přes attach, použije první pin
  void attach(const std::vector<int>& pins) override {
    if (!pins.empty()) {
      _pin = pins[0];
      if (_pin >= 0) {
        pinMode(_pin, INPUT);
        ensureIrRecv_();
        startTaskIfNeeded_();
      }
    }
  }

  // uvolnění pinu a zdrojů
  void detach() override;

private:
  // helpery
  void ensureIrRecv_();
  void startTaskIfNeeded_();
  static void taskEntry_(void* pv);
  void taskLoop_();
  static String toHex10_(uint32_t v);

  int            _pin;
  unsigned long  _dedupMs;

  
  IRrecv*         _irrecv = nullptr;          // z IRremoteESP8266
  void*           _task   = nullptr;          // TaskHandle_t jako void*
  decode_results* _res    = nullptr;          // z IRremoteESP8266

  // sdílená data
  volatile uint32_t      _latestVal   = 0;
  volatile bool          _haveAny     = false;
  volatile unsigned long _lastStoreMs = 0;

  uint32_t       _lastValRx = 0;
  unsigned long  _lastMsRx  = 0;
};
