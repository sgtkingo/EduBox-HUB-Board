#include <Senzor_Encoder.hpp>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

// Counts per physical detent (většina mechanických enkodérů = 4)
static const int ENC_CPD = 2;

portMUX_TYPE Rencoder::s_mux = portMUX_INITIALIZER_UNLOCKED;
bool Rencoder::s_isrInstalled = false;

// kvadraturní lookup (prev<<2 | curr) -> delta
static const int8_t qlut[16] = {
  0,  -1,  +1,  0,
  +1,  0,   0, -1,
  -1,  0,   0, +1,
  0,  +1, -1,  0
};

static inline bool hasInternalPullup(int pin) {
  return (pin >= 0 && pin <= 33); // 34..39 nemají interní pull-up
}

void IRAM_ATTR Rencoder::gpio_isr_router(void* arg) {
  reinterpret_cast<Rencoder*>(arg)->handleEdgeISR();
}

void IRAM_ATTR Rencoder::handleEdgeISR() {
  int a = gpio_get_level((gpio_num_t)_pinA);
  int b = gpio_get_level((gpio_num_t)_pinB);
  uint8_t curr = (a << 1) | b;
  uint8_t prev = _prevState;
  uint8_t idx = (prev << 2) | curr;
  int8_t d = qlut[idx];
  if (d) {
    portENTER_CRITICAL_ISR(&s_mux);
    _ticks += d;
    portEXIT_CRITICAL_ISR(&s_mux);
  }
  _prevState = curr;
}

bool Rencoder::startISR() {
  if (_pinA < 0 || _pinB < 0) return false;

  if (hasInternalPullup(_pinA)) pinMode(_pinA, INPUT_PULLUP);
  else                          pinMode(_pinA, INPUT);
  if (hasInternalPullup(_pinB)) pinMode(_pinB, INPUT_PULLUP);
  else                          pinMode(_pinB, INPUT);

  if (!s_isrInstalled) {
    gpio_install_isr_service(0);
    s_isrInstalled = true;
  }

  int a = gpio_get_level((gpio_num_t)_pinA);
  int b = gpio_get_level((gpio_num_t)_pinB);
  _prevState = (a << 1) | b;

  gpio_pad_select_gpio((gpio_num_t)_pinA);
  gpio_set_intr_type((gpio_num_t)_pinA, GPIO_INTR_ANYEDGE);
  gpio_isr_handler_add((gpio_num_t)_pinA, &Rencoder::gpio_isr_router, this);

  gpio_pad_select_gpio((gpio_num_t)_pinB);
  gpio_set_intr_type((gpio_num_t)_pinB, GPIO_INTR_ANYEDGE);
  gpio_isr_handler_add((gpio_num_t)_pinB, &Rencoder::gpio_isr_router, this);

  // reset counters
  portENTER_CRITICAL(&s_mux);
  _ticks = 0;
  portEXIT_CRITICAL(&s_mux);
  _position = 0;
  _unit = -1;
  _last = 0;
  return true;
}

void Rencoder::stopISR() {
  if (_pinA >= 0) {
    gpio_isr_handler_remove((gpio_num_t)_pinA);
    gpio_set_intr_type((gpio_num_t)_pinA, GPIO_INTR_DISABLE);
  }
  if (_pinB >= 0) {
    gpio_isr_handler_remove((gpio_num_t)_pinB);
    gpio_set_intr_type((gpio_num_t)_pinB, GPIO_INTR_DISABLE);
  }
}

bool Rencoder::init() {
  return startISR();
}

void Rencoder::detach() {
  stopISR();
  if (_pinA >= 0) pinMode(_pinA, INPUT);
  if (_pinB >= 0) pinMode(_pinB, INPUT);
}

void Rencoder::reset() {
  portENTER_CRITICAL(&s_mux);
  _ticks = 0;
  portEXIT_CRITICAL(&s_mux);
  _position = 0;
  _last = 0;
}

std::vector<KV> Rencoder::update() {
  std::vector<KV> kv;
  int64_t local_ticks;
  portENTER_CRITICAL(&s_mux);
  local_ticks = _ticks;
  portEXIT_CRITICAL(&s_mux);

  long detents = (long)(local_ticks / ENC_CPD);
  _position = detents * _dir;

  String alarm = "OK";
  if (_position < _lLimit)      alarm = "LOW";
  else if (_position > _hLimit) alarm = "HIGH";

  kv.push_back({"position", String(_position)});
  kv.push_back({"alarm",    alarm});
  return kv;
}
