
#include <Senzor_IRrx.hpp>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

String IRrx::toHex10_(uint32_t v) {
  char out[11];
  snprintf(out, sizeof(out), "0x%08lX", (unsigned long)v);
  return String(out);
}

void IRrx::detach() {
  if (_irrecv) {
    delete _irrecv;
    _irrecv = nullptr;
  }
  // uvolnit další zdroje a resetovat stav
  _task = nullptr;
  _res = nullptr;
  _haveAny = false;
  _latestVal = 0;
  _lastStoreMs = 0;
  _pin = -1;
}

bool IRrx::init() {
  if (_res == nullptr) {
    _res = new decode_results();      
  }
  startTaskIfNeeded_();
  return true;
}

void IRrx::reset() {
  if (_task) { vTaskDelete((TaskHandle_t)_task); _task = nullptr; }
  if (_irrecv) { delete _irrecv; _irrecv = nullptr; }
  if (_res) { delete _res; _res = nullptr; }

  _haveAny = false; _latestVal = 0; _lastStoreMs = 0;
  _lastValRx = 0; _lastMsRx = 0;
}

void IRrx::ensureIrRecv_() {
  if (_irrecv == nullptr) {
    _irrecv = new IRrecv(_pin);       // <<< globální typ
    _irrecv->setTolerance(40);
    _irrecv->setUnknownThreshold(12);
    _irrecv->enableIRIn();
  }
}

void IRrx::taskEntry_(void* pv) {
  static_cast<IRrx*>(pv)->taskLoop_();
}

void IRrx::startTaskIfNeeded_() {
  ensureIrRecv_();
  if (_task == nullptr) {
    xTaskCreatePinnedToCore(&IRrx::taskEntry_, "IRReader",
                            4096, this, 1, (TaskHandle_t*)&_task, 0);
  }
}

void IRrx::taskLoop_() {
  ensureIrRecv_();

  for (;;) {
    if (_irrecv && _res && _irrecv->decode(_res)) {
      const uint32_t v = (uint32_t)_res->value;

      const bool isRepeat    = _res->repeat;
      const bool tooSoonSame = (millis() - _lastMsRx) < _dedupMs && v == _lastValRx;

      if (!isRepeat && !tooSoonSame) {
        _latestVal   = v;    // 32bit je na ESP32 atomický
        _haveAny     = true;
        _lastStoreMs = millis();

        _lastValRx = v;
        _lastMsRx  = millis();
      }
      _irrecv->resume();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

std::vector<KV> IRrx::update() {
  startTaskIfNeeded_();
  const uint32_t v = _latestVal;
  return { {"code", toHex10_(v)} };
}


  
