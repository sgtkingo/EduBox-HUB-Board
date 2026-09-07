#include <Senzor_Antc.hpp>
#include <cmath>

static inline int readAveraged(int pin, int n, int dlyMs) {
  long sum = 0;
  for (int i=0; i<n; ++i) { sum += analogRead(pin); delay(dlyMs); }
  return int(sum / n);
}

float Antc::computeTemp(int adc, int resBits) {
  constexpr double Vref = 3.3;
  const double vo = double(adc) * (std::pow(2.0, resBits) - 1.0);
  const double v = double(adc) * (Vref / (std::pow(2.0, resBits) - 1.0));
  const double r2 = 10000.0 * (v / (Vref - v));
  float t = std::log(r2);
  t = 1.0f / (0.001129148f + 0.000234125f * t + 0.0000000876741f * t * t * t);
  return t - 276.8f;
}

std::vector<KV> Antc::update() {
  const int samples = samplesFor(_filter);
  const int adc = readAveraged(_pin, samples, 3);
  const float tempC = computeTemp(adc, _res);
  return { KV{"temp", String(tempC,1)} };
}
