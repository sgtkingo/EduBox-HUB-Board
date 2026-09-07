#include <Senzor_GP2Y0A21YK0F.hpp>
#include <GP2Y0A21YK0F.h> 

// Pomocná: saturace do <min,max>
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool GP2Y0A21YK0F::init() {
  DMSU sharp(_pin);
  // čti v cm (0) pro jednoduché ověření funkčnosti
  float dist = sharp.read(0);
  // funguje, pokud je v typickém rozsahu 20–80 cm
  return (dist >= 20.0f && dist <= 80.0f);
}

std::vector<KV> GP2Y0A21YK0F::update() {
  std::vector<KV> kv;

  DMSU sharp(_pin);
  // 0 = cm, 1 = mm (zachováváme původní chování)
  float distance = sharp.read(_unit);

  // Ořez extrémů <20 → 19, >80 → 81 (v dané jednotce!)
  if (_unit == 0) {
    // cm
    distance = clampf(distance, 19.0f, 81.0f);
  } else {
    // mm (20–80 cm == 200–800 mm), ořez na 190/810 mm
    distance = clampf(distance, 190.0f, 810.0f);
  }

  String alarm = "OK";
  if (distance < _lAlarm)      alarm = "LOW";
  else if (distance > _hAlarm) alarm = "HIGH";

  kv.push_back({"distance", String(distance, 1)}); 
  kv.push_back({"alarm",    alarm});
  return kv;
}
