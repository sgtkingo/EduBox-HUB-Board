#include <Arduino.h>
#include <BuzzP.hpp>

static inline int chForPin(int pin) { return (pin & 0x07); }

void BuzzP_control(int pin, int freq, int duration){
 tone(pin,freq,duration);
}


void BuzzP_reset(int pin){
 analogWrite(pin,0);
} 


bool BuzzP::init() {
  // Inicializace pinu jako výstup
  if (_freq <= 0) _freq = 1;          //1 Hz ~ „ticho“
  const int ch = chForPin(_pin);

  //Reinit buzzeru
  ledcDetachPin(_pin);                   
  ledcSetup(ch, _freq, 8);            // 8bit rozlišení 
  ledcAttachPin(_pin, ch);
  ledcWriteTone(ch, _freq);

  if (_duration > 0) {
    delay(_duration);
    ledcWrite(ch, 0);                 
    ledcDetachPin(_pin);              
  }
  return _pin >= 0;
}
