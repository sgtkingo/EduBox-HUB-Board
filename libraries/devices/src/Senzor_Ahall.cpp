#include <Senzor_Ahall.hpp>


short val;

std::vector<KV> Ahall::update() {
  analogReadResolution(_res);
  _val = analogRead(_pin);

  String polarity;
  if (_val > _hLimit)       polarity = "SOUTH";
  else if (_val < _lLimit)  polarity = "NORTH";
  else                      polarity = "NO MAGNET";

  std::vector<KV> kv;
  kv.push_back({"val",      String(_val)});
  kv.push_back({"polarity", polarity});
  return kv;
}

