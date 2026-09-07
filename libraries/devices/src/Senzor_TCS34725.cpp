#include <Senzor_TCS34725.hpp>

// Jediná instance I2C a TCS34725 pro tento senzor
TwoWire I2C(0);
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_4X);

static uint8_t mapItime_(int itime_ms) {
  switch (itime_ms) {
    case 2:   return TCS34725_INTEGRATIONTIME_2_4MS;
    case 50:  return TCS34725_INTEGRATIONTIME_50MS;
    case 101: return TCS34725_INTEGRATIONTIME_101MS;
    case 199: return TCS34725_INTEGRATIONTIME_199MS;
    case 300: return TCS34725_INTEGRATIONTIME_300MS;
    case 401: return TCS34725_INTEGRATIONTIME_401MS;
    case 499: return TCS34725_INTEGRATIONTIME_499MS;
    case 600: default: return TCS34725_INTEGRATIONTIME_600MS;
  }
}
static tcs34725Gain_t mapGain_(int g) {
  switch (g) {
    case 1:  return TCS34725_GAIN_1X;
    case 4:  return TCS34725_GAIN_4X;
    case 16: return TCS34725_GAIN_16X;
    case 60: default: return TCS34725_GAIN_60X;
  }
}

bool TCS34725::init() {
  I2C.begin(_sda, _scl);
  
  // Před voláním tcs.begin() zkontroluj, zda zařízení odpovídá
  I2C.beginTransmission(0x29);
  uint8_t wireErr = I2C.endTransmission();
  if (wireErr != 0) {
    _tcsEnabled = false;
    return false;  // zařízení není na sběrnici, neloguj chybu
  }

  if (!tcs.begin(0x29, &I2C)) {
    _tcsEnabled = false;
    return false;
  }

  applyConfig_();
  tcs.enable();

  _tcsEnabled = true;
  _readyAtMs  = millis() + _itime + 5;
  _blockFirst = true;

  return true;
}


void TCS34725::reset() {
  tcs.begin(0x29, &I2C);
  applyConfig_();
}

void TCS34725::applyConfig_() {
  // mapování parametrů na enumy knihovny
  tcs.setIntegrationTime(mapItime_(_itime));
  tcs.setGain(mapGain_(_gain));
}

std::vector<KV> TCS34725::update() {
  std::vector<KV> kv;

  // ID check – accept pro 0x44 i 0x4D
  uint8_t id = tcs.read8(TCS34725_ID);
  if (id != 0x44 && id != 0x4D) {
    if (!init()) return kv;            
    return kv;                         
  }

  // Warm-up: ještě neuplynula integrační doba?
  long msLeft = (long)(_readyAtMs - millis());
  if (!_tcsEnabled || msLeft > 0) {
    if (_blockFirst) {
      // poprvé po CONNECT/RESET se čeká, další volání vrací data
      if (msLeft > 0) delay(msLeft);
      _blockFirst = false;
    } else {
      return kv;   
    }
  }

  uint16_t r,g,b,c;
  tcs.getRawData(&r,&g,&b,&c);

  // Auto-recover
  if (r==0 && g==0 && b==0 && c==0) {
    tcs.disable(); tcs.enable();
    _readyAtMs  = millis() + _itime + 5;
    _blockFirst = true;                // při re-enable zase blokuje první
    return kv;                         
  }

  // normalizace a korekce barev (klidně upravit dle referenčních hodnot)
  if (c == 0) c = 1;
  float rn = (float)r / (c + 1);
  float gn = (float)g / (c + 1);
  float bn = (float)b / (c + 1);

  const float R_CORR = 0.7f, G_CORR = 1.1f, B_CORR = 1.7f;
  rn *= R_CORR; gn *= G_CORR; bn *= B_CORR;
  float maxRGB = max(rn, max(gn, bn));
  if (maxRGB > 1.0f) { rn/=maxRGB; gn/=maxRGB; bn/=maxRGB; }

  kv.push_back({"R", String((uint8_t)constrain(rn*255.0f,0.0f,255.0f))});
  kv.push_back({"G", String((uint8_t)constrain(gn*255.0f,0.0f,255.0f))});
  kv.push_back({"B", String((uint8_t)constrain(bn*255.0f,0.0f,255.0f))});
  return kv;
}


