#include <Senzor_BMP.hpp>
#include <Wire.h>
#include <Adafruit_BMP280.h>

extern TwoWire I2C;
Adafruit_BMP280 bmp(&I2C);

// --- pomocné mapování nastavení ---
Adafruit_BMP280::sensor_sampling BMP280::mapOs(int v) {
  if (v >= 16) return Adafruit_BMP280::SAMPLING_X16;
  if (v >= 8)  return Adafruit_BMP280::SAMPLING_X8;
  if (v >= 4)  return Adafruit_BMP280::SAMPLING_X4;
  if (v >= 2)  return Adafruit_BMP280::SAMPLING_X2;
  return Adafruit_BMP280::SAMPLING_X1;
}
Adafruit_BMP280::sensor_filter BMP280::mapFilter(int v) {
  if (v >= 16) return Adafruit_BMP280::FILTER_X16;
  if (v >= 8)  return Adafruit_BMP280::FILTER_X8;
  if (v >= 4)  return Adafruit_BMP280::FILTER_X4;
  if (v >= 2)  return Adafruit_BMP280::FILTER_X2;
  return Adafruit_BMP280::FILTER_OFF;
}

// init senzoru
bool BMP280::init() {
  I2C.end();
  delay(2);
  I2C.begin(_sda, _scl);
  return bmp.begin(0x76);
}

void BMP280::reset() {
  bmp.begin(0x76);
}
// konfugurace senzoru
void BMP280::config(Param* params, int count) {
  for (int i = 0; i < count; ++i) {
    String k = params[i].key;
    k.trim();
    k.toLowerCase();
    if      (k == "os_temp")  _os_temp  = params[i].value.toInt();
    else if (k == "os_press") _os_press = params[i].value.toInt();
    else if (k == "filter")   _filter   = params[i].value.toInt();
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    mapOs(_os_temp),
    mapOs(_os_press),
    mapFilter(_filter),
    Adafruit_BMP280::STANDBY_MS_500
  );
}

// měření hodnot
std::vector<KV> BMP280::update() {
  // pozn.: původní kód měl kalibrační konstantu `cal = 0`; zachovávám
  const float t = bmp.readTemperature();           // °C
  const float p = bmp.readPressure() / 100.0f;     // Pa -> hPa

  std::vector<KV> kv;
  kv.push_back({"temp",  String(t, 1)});
  kv.push_back({"press", String(p, 1)});
  return kv;
}
