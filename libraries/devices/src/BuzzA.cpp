#include <Arduino.h>
#include <BuzzA.hpp>


void BuzzA_control(int pin, bool control){
 digitalWrite(pin,control);
}


void BuzzA_reset(int pin){
 digitalWrite(pin, LOW);
}
