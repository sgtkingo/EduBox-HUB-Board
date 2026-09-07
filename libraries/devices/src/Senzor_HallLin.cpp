#include <Senzor_HallLin.hpp>
#include <math.h>

// inicializace senzoru
bool HallLin::init() {
  int raw1 = analogRead(_pin);
  delay(10);
  int raw2 = analogRead(_pin);

  float voltage1 = raw1 * (3.3f / 4095.0f);
  float voltage2 = raw2 * (3.3f / 4095.0f);
  float diff = fabsf(voltage1 - voltage2);

  return (voltage1 >= 0.9f && voltage1 <= 2.0f) && (diff < 0.3f);
}

// měření hodnot
std::vector<KV> HallLin::update() {
  std::vector<KV> kv;

  analogReadResolution(_res);
  const int raw = analogRead(_pin);

  float result = 0.0f;
  String key;

  if (_unit == "ADC") {
    result = static_cast<float>(raw);
    key = "ADC";
  } else {
    const float Vref = 3.3f;
    const int   maxADC = (1 << _res) - 1;
    const float voltage = (static_cast<float>(raw) / static_cast<float>(maxADC)) * Vref;

    if (_unit == "Induction") {
      const float zeroOffset  = Vref / 2.0f;   // ~1.65 V
      const float sensitivity = 0.0025f;       // V/G
      result = ((voltage - zeroOffset) / sensitivity) * 0.1f;  // mT
      key = "Induction";
      
    } else {
      result = voltage;
      key = "Voltage";
    }
  }

  kv.push_back({ key, String(result, 2) });
  return kv;
}
