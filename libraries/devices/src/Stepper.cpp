#include <Stepper.hpp>
#include <CheapStepper.h>

// Validace pinů
static inline bool validOutPin_(int p) {
  if (p < 0) return false;
  return true;
}

// Uvolni piny
void Stepper::releasePins_() {
  if (_pin1 >= 0) pinMode(_pin1, INPUT);
  if (_pin2 >= 0) pinMode(_pin2, INPUT);
  if (_pin3 >= 0) pinMode(_pin3, INPUT);
  if (_pin4 >= 0) pinMode(_pin4, INPUT);
}

// Nastavení driveru pokud jsou správné piny
void Stepper::ensureDriver_() {
  if (_stp) return;
  if (!validOutPin_(_pin1) || !validOutPin_(_pin2) ||
      !validOutPin_(_pin3) || !validOutPin_(_pin4)) {
    return; 
  }

  pinMode(_pin1, OUTPUT);
  pinMode(_pin2, OUTPUT);
  pinMode(_pin3, OUTPUT);
  pinMode(_pin4, OUTPUT);

  _stp = new CheapStepper(_pin1, _pin2, _pin3, _pin4);
  _stp->setRpm(_rpm);
  _stp->stop(); // drží cívky vypnuté, dokud nepřijde příkaz
}

// připojení – nastav piny a vytvoř driver
void Stepper::attach(const std::vector<int>& pins) {
  // vypnout a uvolnit starý driver
  if (_stp) { _stp->stop(); delete _stp; _stp = nullptr; }

  if (pins.size() > 0) _pin1 = pins[0];
  if (pins.size() > 1) _pin2 = pins[1];
  if (pins.size() > 2) _pin3 = pins[2];
  if (pins.size() > 3) _pin4 = pins[3];

  ensureDriver_();
}

// odpojení – vypnout a uvolnit piny
void Stepper::detach() {
  _stp->stop();
  if (_stp) { delete _stp; _stp = nullptr; }
  releasePins_();
}

bool Stepper::init() {
  // opakovaná ochrana – když attach nedodal všechny piny, nic se nestane
  if (!_stp) ensureDriver_();
  return _stp != nullptr;
}

void Stepper::control(Param* params, int count) {
  for (int i = 0; i < count; ++i) {
    String k = params[i].key;
    k.trim();
    k.toLowerCase();
    if      (k == "angle") _angle = params[i].value.toInt();
    else if (k == "dir") {
       String v = params[i].value;
       _dir = (v == "true" || v == "1");
    }
    else if (k == "rpm")   _rpm   = params[i].value.toInt();
  }

  ensureDriver_();
  if (_stp) _stp->setRpm(_rpm);

  int angleAbs = abs(_angle);
  if (_stp && angleAbs > 0) {
    _currentPos += _dir ? angleAbs : -angleAbs;
    _stp->moveDegrees(_dir, angleAbs);
    _stp->stop();
  }
}

void Stepper::reset() {
  if (_currentPos != 0) {
    ensureDriver_();
    if (_stp) {
      int backAngle = abs(_currentPos);
      bool backDir  = (_currentPos > 0) ? false : true;
      int oldRpm = _rpm;
      _stp->setRpm(16);
      _stp->moveDegrees(backDir, backAngle);
      _stp->setRpm(oldRpm);
      _stp->stop();
    }
    _currentPos = 0;
  }
}
