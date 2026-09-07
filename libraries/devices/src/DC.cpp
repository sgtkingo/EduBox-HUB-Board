#include <DC.hpp>
#include "Arduino.h"


static int pwmPin = -1;
static bool state = false;

void DC_control(int pin, int Speed, bool state) {
  pwmPin = pin;
  state = state;    //on = true, off = false

  pinMode(pwmPin, OUTPUT);

  Speed = constrain(Speed, 0, 100);
  int pwmValue = map(Speed, 0, 100, 0, 255); // převod na rozsah PWM

  if (state) {
    analogWrite(pwmPin, pwmValue);
  } else {
    analogWrite(pwmPin, 0); // vypnout motor
  }

}

void DC_reset() {

    analogWrite(pwmPin, 0); // vypnout
    state = false;
  
}
