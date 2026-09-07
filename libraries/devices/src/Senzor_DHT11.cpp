#include <Arduino.h>
#include <cmath>
#include <Senzor_DHT11.hpp>


// měření hodnot
std::vector<KV> DHT11x::update() {
  std::vector<KV> kv;
  if (!_dht) {
    kv.push_back({"humi", "nan"});
    kv.push_back({"temp", "nan"});
    return kv;
  }

  float h = _dht->readHumidity();
  float t = _dht->readTemperature(_unitF);    // _unitF: false=°C, true=°F

  if (_useHI) {
    t = _dht->computeHeatIndex(t, h, _unitF); // Heat Index v odpovídající jednotce
  }

  kv.push_back({"humi", isnan(h) ? String("nan") : String(h, 1)});
  kv.push_back({"temp", isnan(t) ? String("nan") : String(t, 1)});
  return kv;
}

// inicializace senzoru 
bool DHT11x::init() {
  if (!_dht) return false;
  _dht->begin();
  return true;
}

// reset senzoru
void DHT11x::reset() {
  if (_dht) _dht->begin();
}

