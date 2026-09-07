#include <Senzor_PHresistance.hpp>
#include <math.h>

bool PHresistance::init() {
  // raw check, stačí aby value nebyla 0
  return analogRead(_pin) > 100;
}

std::vector<KV> PHresistance::update() {
  analogReadResolution(_res);
  const int raw = analogRead(_pin);

  const float Vref   = 3.3f;
  const float Rfixed = 10000.0f;

  const float maxAdc = (float)((1UL << _res) - 1UL);
  const float voltage = (maxAdc > 0.0f) ? (raw / maxAdc) * Vref : 0.0f;

  // ochrana proti dělení nulou
  float R = 0.0f;
  if (voltage >= Vref - 1e-6f) {
    R = 1e6f; // velké číslo => velmi nízké světlo (R -> vysoké)
  } else if (voltage <= 1e-6f) {
    R = 1e-3f; // téměř nula => velmi vysoké světlo
  } else {
    R = (voltage * Rfixed) / (Vref - voltage);
  }

  // lux aproximace + gain
  float lux = _gain * (455.0f * powf(Rfixed / R, 0.68f));
  if (!isfinite(lux)) lux = 0.0f;
  if (lux < 0.0f)     lux = 0.0f;

  return { {"intensity", String(lux, 1)} };
}
