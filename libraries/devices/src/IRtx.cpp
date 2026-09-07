#include <IRtx.hpp>
#include <IRremoteESP8266.h>
#include <IRsend.h>

static IRsend* s_irsend = nullptr;
static int s_currentPin = -1;
static bool s_ready = false;

static void ensureIrSend(int pin) {
  if (s_irsend == nullptr || s_currentPin != pin) {
    if (s_irsend) { delete s_irsend; s_irsend = nullptr; }
    s_irsend = new IRsend(pin);
    s_currentPin = pin;
    s_ready = false;
  }
  if (!s_ready) {
    s_irsend->begin();
    s_ready = true;
  }
}

void IRtx_control(int pin, uint32_t code) {
  ensureIrSend(pin);
  if (s_irsend) {
    s_irsend->sendNEC(code, 32);  
  }
}

void IRtx_reset(int pin) {
  if (s_irsend) {
    delete s_irsend;
    s_irsend = nullptr;
  }
  s_currentPin = -1;
  s_ready = false;
}
