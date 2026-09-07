#include <Senzor_Heartbeat.hpp>

bool Heartbeat::init() {
  const int samples = 10;
  int valid = 0;
  for (int i = 0; i < samples; ++i) {
    int val = analogRead(_pin);
    delay(10);
    if (val > 200 && val < 650) ++valid;   // stejné prahy jako dřív
  }
  return (valid > samples / 2);
}

std::vector<KV> Heartbeat::update() {
  const int samp_siz = 4;               // vyhlazovací okno
  const int threshold = 4;              // potřebné vzestupy pro detekci
  const int min_beat_interval = 300;    // ms mezi údery (debounce)

  float reads[samp_siz]{};
  float sum = 0.0f;
  int   ptr = 0;

  float before = 0.0f, avg = 0.0f;
  bool  rising = false;
  int   rise_count = 0;

  unsigned long last_beat = 0;
  int beat_count = 0;
  const unsigned long start_time = millis();

  while (millis() - start_time < (unsigned long)_time) {
    // vzorkování ~20 ms pro potlačení šumu
    float reader = 0.0f; int n = 0;
    const unsigned long t0 = millis();
    do { reader += analogRead(_pin); ++n; } while (millis() - t0 < 20);
    reader /= (n > 0 ? n : 1);

    // klouzavý průměr
    sum -= reads[ptr]; sum += reader;
    reads[ptr] = reader;
    ptr = (ptr + 1) % samp_siz;
    avg = sum / samp_siz;

    // detekce vzestupu
    if (avg > before) {
      if (++rise_count > threshold && !rising) {
        const unsigned long now = millis();
        if (now - last_beat > (unsigned long)min_beat_interval) {
          ++beat_count;
          last_beat = now;
        }
        rising = true;
      }
    } else {
      rising = false;
      rise_count = 0;
    }

    before = avg;
  }

  const float duration_s = (millis() - start_time) / 1000.0f;
  const float bpm = (beat_count > 0 && duration_s > 0.0f)
                    ? (beat_count * 60.0f / duration_s)
                    : 0.0f;

  return { {"bpm", String(bpm, 0)} };
}
