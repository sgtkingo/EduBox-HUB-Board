#include <Arduino.h>
#include <BuzzP.hpp>

void BuzzP_control(int pin, int freq, int duration){
 tone(pin,freq,duration);
}


void BuzzP_reset(int pin){
 analogWrite(pin,0);
} 


bool BuzzP::init() {
  // Inicializace pinu jako výstup
  if (_pin < 0) return false;
  noTone(_pin);
  ledcDetachPin(_pin);
  digitalWrite(_pin, LOW);
  pinMode(_pin, OUTPUT);
  return true; // CONNECT is silent; tone() starts only on CONTROL.
}
