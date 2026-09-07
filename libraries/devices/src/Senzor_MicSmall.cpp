#include <Senzor_MicSmall.hpp>
#include <math.h>

// referenční hladina
static float REF_BASE = 0.0f;

bool MicSmall::init() {
  // výpočet několika vzorků pro referenční hladinu
  const int samples = 10;
  analogReadResolution(_res);

  long sum = 0;
  int minVal = (1 << _res) - 1;
  int maxVal = 0;

  for (int i = 0; i < samples; ++i) {
    int val = analogRead(_pin);
    sum += val;
    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;
    delay(5);
  }

  // průměr jako referenční hladina
  REF_BASE = (float)sum / (float)samples;
  if (!REF_BASE <= 0.0f) return false;
  return true;
}

std::vector<KV> MicSmall::update() {
  analogReadResolution(_res);

  float maxDiff = 0.0f;
  const unsigned long start = millis();

  while (millis() - start < (unsigned long)_time) {
    const float sample = analogRead(_pin);
    const float diff   = fabsf(sample - (float)REF_BASE);
    if (diff > maxDiff) maxDiff = diff;
  }

  const float volume = 20.0f * log10f(maxDiff + 1.0f);

  return { {"volume", String(volume, 1)} };
}
