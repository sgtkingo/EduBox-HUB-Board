#include <Senzor_HC_SR04.hpp>
#include <NewPing.h>

//inicializace senzoru
bool HCSR04::init() {
  NewPing sonar(_trig, _echo, _limit);
  int dist = sonar.ping_cm();
  return (dist > 2);                // stejné chování jako dřív
}


//měření hodnot 
std::vector<KV> HCSR04::update() {
  std::vector<KV> kv;

  NewPing sonar(_trig, _echo, _limit);
  delay(_delayMs);                  // ~20 pingů/s; 29 ms je minimum

  // původní výpočet: us / 57.0f -> cm s desetinnými místy
  const unsigned int us = sonar.ping();
  const float dist = us / 57.0f;

  kv.push_back({"distance", String(dist, 1)});
  return kv;
}
