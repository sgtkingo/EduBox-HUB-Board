#include <Senzor_BMP180.hpp>
#include <Adafruit_BMP085.h>

extern TwoWire I2C;
Adafruit_BMP085 bmp180;

// inicializace senzoru (sběrnice)
bool BMP180::init() {
  I2C.end();
  delay(2);
  I2C.begin(_sda, _scl);
  return bmp180.begin(0x77, &I2C);
}

// reset senzoru (sběrnice)
void BMP180::reset() {
  // re-init senzoru na stejné adrese a sběrnici
  bmp180.begin(0x77, &I2C);
}

//měření hodnot
std::vector<KV> BMP180::update() {
  // čtení surového tlaku (Pa)
  const float pressure_raw = bmp180.readPressure();
  
  // aplikace kalibračního offsetu a gain faktoru (v Pa)
  float pressure_pa = (pressure_raw + CAL_OFFSET * 100.0f) * _gain;
  
  // výpočet nadmořské výšky (vstup v Pa)
  const float altitude = bmp180.readAltitude(pressure_pa);
  
  // převod tlaku na hPa pro výstup
  float pressure_hpa = pressure_pa / 100.0f;
  
  std::vector<KV> kv;
  kv.push_back({"press",    String(pressure_hpa, 1)});   // hPa
  kv.push_back({"altitude", String(altitude, 1)});       // m
  return kv;
}
