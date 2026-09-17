#include <Stepper.hpp>
#include <CheapStepper.h>
#include <climits>
#include <algorithm>

// Validace pinů
static inline bool validOutPin_(int p) {
  if (p < 0) return false;
  return true;
}

// Uvolni piny
void Stepper::releasePins_() {
  for (const int pin : {_pin1, _pin2, _pin3, _pin4}) {
    if (pin >= 0) {
      digitalWrite(pin, LOW); // CheapStepper::stop() only cancels steps, not coil power.
      pinMode(pin, INPUT);
    }
  }
  _pin1 = -1;
  _pin2 = -1;
  _pin3 = -1;
  _pin4 = -1;
}

// Nastavení driveru pokud jsou správné piny
void Stepper::ensureDriver_() {
  if (_stp) return;
  if (!validOutPin_(_pin1) || !validOutPin_(_pin2) ||
      !validOutPin_(_pin3) || !validOutPin_(_pin4)) {
    return; 
  }

  for (const int pin : {_pin1, _pin2, _pin3, _pin4}) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }

  _stp = new CheapStepper(_pin1, _pin2, _pin3, _pin4);
  _stp->setRpm(_rpm);
  _stp->stop(); // drží cívky vypnuté, dokud nepřijde příkaz
}

// připojení – nastav piny a vytvoř driver
void Stepper::attach(const std::vector<int>& pins) {
  detach();
  if (pins.size() != requiredPinCount()) return;

  _pin1 = pins[0];
  _pin2 = pins[1];
  _pin3 = pins[2];
  _pin4 = pins[3];
  _angle = 0;
  _dir = _connectDir;
  _rpm = _connectRpm;

  ensureDriver_();
}

// odpojení – vypnout a uvolnit piny
void Stepper::detach() {
  if (_stp) {
    _stp->stop();
    delete _stp;
    _stp = nullptr;
  }
  releasePins_();
  _positionSteps = 0;
}

bool Stepper::init() {
  // opakovaná ochrana – když attach nedodal všechny piny, nic se nestane
  if (!_stp) ensureDriver_();
  return _stp != nullptr;
}

void Stepper::control(Param* params, int count) {
  bool requestedMove = false;
  for (int i = 0; i < count; ++i) {
    String k = params[i].key;
    k.trim();
    k.toLowerCase();
    if (k == "angle") { _angle = params[i].value.toInt(); requestedMove = true; }
    else if (k == "dir") {
       String v = params[i].value;
       _dir = (v == "true" || v == "1");
    }
    else if (k == "rpm")   _rpm   = params[i].value.toInt();
  }

  ensureDriver_();
  if (_stp) _stp->setRpm(_rpm);

  if (_stp && requestedMove) {
    const int64_t magnitude = _angle < 0 ? -static_cast<int64_t>(_angle) : _angle;
    const int steps = static_cast<int>(std::min<int64_t>(magnitude * 4096 / 360, INT_MAX));
    _moveDir = _dir;
    _stp->newMove(_dir, steps); // service() advances movement without blocking VSCP.
    if (!steps) {
      for (const int pin : {_pin1, _pin2, _pin3, _pin4}) digitalWrite(pin, LOW);
    }
  }
}

void Stepper::reset() {
  if (_stp) _stp->stop();
  if (_positionSteps != 0) {
    ensureDriver_();
    if (_stp) {
      _moveDir = _positionSteps < 0;
      const int64_t distance = _positionSteps < 0 ? -_positionSteps : _positionSteps;
      _stp->newMove(_moveDir, static_cast<int>(std::min<int64_t>(distance, INT_MAX)));
    }
  }
  if (_stp && _positionSteps == 0) {
    for (const int pin : {_pin1, _pin2, _pin3, _pin4}) digitalWrite(pin, LOW);
  }
}

void Stepper::service() {
  if (!_stp) return;
  const int before = abs(_stp->getStepsLeft());
  _stp->run();
  const int after = abs(_stp->getStepsLeft());
  const int completed = before - after;
  if (completed > 0) _positionSteps += _moveDir ? completed : -completed;
  if (before > 0 && after == 0) {
    for (const int pin : {_pin1, _pin2, _pin3, _pin4}) digitalWrite(pin, LOW);
  }
}
