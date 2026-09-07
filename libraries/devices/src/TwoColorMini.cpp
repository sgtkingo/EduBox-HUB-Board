#include <TwoColor.hpp>
#include <TwoColorMini.hpp>
#include <Arduino.h>

static int s_pinR = -1;
static int s_pinG = -1;

// mapování 0-100 na 0-255 PWM + ořez
static inline int clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

static inline int mapToPWM(int brig01_100) {
  brig01_100 = clamp100(brig01_100);
  return map(brig01_100, 0, 100, 0, 255);
}

void TwoColorMini_setPins(int pinRed, int pinGreen) {
  s_pinR = pinRed;
  s_pinG = pinGreen;
}

void TwoColorMini_control(char color, int Brightness) {

  if (s_pinR < 0 && s_pinG < 0) return;
  if (s_pinR >= 0) pinMode(s_pinR, OUTPUT);
  if (s_pinG >= 0) pinMode(s_pinG, OUTPUT);
  const int pwmValue = mapToPWM(Brightness);

  // zhasni obě větve, ale pouze pokud pin existuje
  if (s_pinR >= 0) analogWrite(s_pinR, 0);
  if (s_pinG >= 0) analogWrite(s_pinG, 0);

  // rozsvit vybranou barvu (pokud má pin)
  if (color == 'r' || color == 'R') {
    if (s_pinR >= 0) analogWrite(s_pinR, pwmValue);
    if (s_pinG >= 0) analogWrite(s_pinG, 0);
  } else if (color == 'g' || color == 'G') {
    if (s_pinG >= 0) analogWrite(s_pinG, pwmValue);
    if (s_pinR >= 0) analogWrite(s_pinR, 0);
  } else {
  }
}

void TwoColorMini_reset() {
  if (s_pinR >= 0) analogWrite(s_pinR, 0);
  if (s_pinG >= 0) analogWrite(s_pinG, 0);
}
