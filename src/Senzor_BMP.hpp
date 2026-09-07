/**
 * @file Senzor_BMP.hpp
 * @brief BMP280 pressure and temperature device declaration.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "Sensor.hpp"
#include <Wire.h>

#include <Adafruit_BMP280.h>
extern TwoWire I2C;

class BMP280 : public Sensor {
public:
  // konstruktor, piny se používají především z attach
  BMP280(int sda = -1, int scl = -1)
    : _sda(sda), _scl(scl), _os_temp(8), _os_press(16), _filter(4) {}

  // přiřazení pinu přes attach, použije první dva piny (SDA, SCL)
  void attach(const std::vector<int>& pins) override {
    if (pins.size() >= 1) _sda = pins[0];
    if (pins.size() >= 2) _scl = pins[1];
  } 

  // uvolnění I2C pinu a ukončení I2C rozhraní
  void detach() override {
    if (_sda >= 0) pinMode(_sda, INPUT);
    if (_scl >= 0) pinMode(_scl, INPUT);
    I2C.end();
  }

  bool init() override;
  std::vector<KV> update() override;
  void reset() override;
  DeviceType deviceType() const override { return DeviceType::Bmp280; }

  // konfigurační parametry: oversampling teploty/tlaku a filtr
  void config(Param* params = nullptr, int count = 0) override;

private:
  // pomocné mapování na enumy knihovny
  static Adafruit_BMP280::sensor_sampling mapOs(int v);
  static Adafruit_BMP280::sensor_filter   mapFilter(int v);

  int _sda;        
  int _scl;        
  int _os_temp;    // oversampling teploty
  int _os_press;   // oversampling tlaku
  int _filter;     // filtr
};
