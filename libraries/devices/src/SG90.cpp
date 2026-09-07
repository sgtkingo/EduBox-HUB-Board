#include <SG90.hpp>

static int s_pin = -1;

// interní pomocná servo instance
static Servo s_globalServo;
static int   s_lastAngle = 0;

// nastavení pinu
void SG90_setPin(int pin) {
  s_pin = pin;
}

// převod rychlosti (0..100) na zpoždění mezi kroky v ms
static inline int speedToDelayMs_(int speed) {
  speed = constrain(speed, 0, 100);
  return map(speed, 0, 100, 50, 0);
}

// funkce pro ovládání serva s plynulým pohybem
static void control_(Servo &s, int start, int end, int speedMs) {
  start = constrain(start, 0, 180);
  end   = constrain(end,   0, 180);
  if (speedMs <= 0) { s.write(end); return; }
  int step = (start < end) ? 1 : -1;
  for (int pos = start; pos != end; pos += step) {
    s.write(pos);
    delay(speedMs);
  }
  s.write(end);
}

// konfigurace serva
void SG90_control(int angle, int speed) {
  if (s_pin < 0) return;        
  angle = constrain(angle, 0, 180);
  int speedMs = speedToDelayMs_(speed);

  if (!s_globalServo.attached()) s_globalServo.attach(s_pin);
  else {
    s_globalServo.detach();
    s_globalServo.attach(s_pin);
  }

  if (angle != s_lastAngle) {
    control_(s_globalServo, s_lastAngle, angle, speedMs);
    s_lastAngle = angle;
  } else {
    s_globalServo.write(angle);
  }
}
