#include <RGB.hpp>


static int s_pinR = -1;
static int s_pinG = -1;
static int s_pinB = -1;

void RGB_setPins(int pinR, int pinG, int pinB) {
  s_pinR = pinR;
  s_pinG = pinG;
  s_pinB = pinB;
  if (s_pinR >= 0) pinMode(s_pinR, OUTPUT);
  if (s_pinG >= 0) pinMode(s_pinG, OUTPUT);
  if (s_pinB >= 0) pinMode(s_pinB, OUTPUT);
}

static inline int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

void RGB_control(int BrigR, int BrigG, int BrigB) {
  if (s_pinR < 0 && s_pinG < 0 && s_pinB < 0) return;

  BrigR = clamp100(BrigR);
  BrigG = clamp100(BrigG);
  BrigB = clamp100(BrigB);

  int pwmR = map(BrigR, 0, 100, 0, 255);
  int pwmG = map(BrigG, 0, 100, 0, 255);
  int pwmB = map(BrigB, 0, 100, 0, 255);

  // agregace podle fyzického pinu (každý pin se zapisuje pouze jednou, používá se max kanálů)
  int pins[3] = { s_pinR, s_pinG, s_pinB };
  int pwms[3] = { pwmR, pwmG, pwmB };

  int outPins[3] = { -1, -1, -1 };
  int outPwms[3] = { 0, 0, 0 };
  int outCount = 0;

  for (int i = 0; i < 3; ++i) {
    int p = pins[i];
    if (p < 0) continue;
    int idx = -1;
    for (int j = 0; j < outCount; ++j) if (outPins[j] == p) { idx = j; break; }
    if (idx >= 0) {
      if (pwms[i] > outPwms[idx]) outPwms[idx] = pwms[i];
    } else if (outCount < 3) {
      outPins[outCount] = p;
      outPwms[outCount] = pwms[i];
      ++outCount;
    }
  }

  for (int i = 0; i < outCount; ++i) {
    int p = outPins[i];
    int v = outPwms[i];
    pinMode(p, OUTPUT);
    analogWrite(p, v);
  }
}

void RGB_reset() {
  if (s_pinR >= 0) { analogWrite(s_pinR, 0); digitalWrite(s_pinR, LOW); }
  if (s_pinG >= 0) { analogWrite(s_pinG, 0); digitalWrite(s_pinG, LOW); }
  if (s_pinB >= 0) { analogWrite(s_pinB, 0); digitalWrite(s_pinB, LOW); }
}
