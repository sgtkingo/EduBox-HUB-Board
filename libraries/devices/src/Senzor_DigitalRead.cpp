#include <Senzor_DigitalRead.hpp>

std::vector<KV> SensorDigitalRead::update() {
  const int digitalValue = digitalRead(_pin);

  std::vector<KV> kv;
  kv.push_back({"state", String(digitalValue)});
  return kv;
}
