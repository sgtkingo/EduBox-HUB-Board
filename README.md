# mtasenzory
M-TA univerzalni program pro cteni dat ze senzoru

[dokumentace](Dokumentace/Senzory_dokumentace.pdf)

# M-TA Senzory (ESP32-S3)  
Univerzální firmware pro čtení dat ze senzorů a ovládání aktuátorů na **ESP32-S3 DevKitC-1** (Arduino/PlatformIO) s jednoduchou integrací přes **VSCP**.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/) 
[![Board](https://img.shields.io/badge/Board-ESP32--S3--DevKitC--1-blue)](#požadavky) 
[![Framework](https://img.shields.io/badge/Framework-Arduino-green)](https://docs.platformio.org/en/latest/frameworks/arduino.html)

---

## ✨ Funkce
- Čtení mnoha analogových i digitálních senzorů (teplota, vlhkost, tlak, barva, vzdálenost, zvuk, IR, …).
- Ovládání aktuátorů (servo, DC motor, krokový motor, bzučáky, RGB LED, laser, IR vysílač).
- Komunikace přes **VSCP API 1.4** (USB Serial a UART2) – společný klientský codec a handler-based server.
- Konfigurovatelné piny a I²C (SDA/SCL) pro snadné zapojení.
- Připraveno pro **PlatformIO** (rychlý build, upload, monitor).

---

## 🧩 Podporované senzory (výběr)
- **Teplota / vlhkost / tlak**: `DHT11`, `BMP180`, `BMP280`, `DS18B20`
- **Analogové senzory**: `NTC` (Senzor_Antc), mikrofony (`MicSmall`, `MicBig`), `PH resistance`
- **Digitální senzory**: `Prakticky veškeré digitální senzory pracující na 3v3/5v`
- **Vzdálenost / barva**: `HC_SR04`, `GP2Y0A21YK0F`, `TCS34725`
- **IR**: příjem (`IRrx`) a vysílání (`IRtx`)
- **Vstupy**: joystick (VRx/VRy/SW), enkodér

> Kompletní seznam viz adresář `src/` (soubory `Senzor_*.hpp/.cpp`) a `main.cpp` (globální registr senzorů).

---

## ⚙️ Podporované aktuátory
- **Servo** `SG90`
- **DC motor** (`DC`)
- **Krokový motor** (`Stepper`)
- **Bzučáky**: aktivní/pasivní (`BuzzA`, `BuzzP`)
- **Světla**: `RGB`, `TwoColor`, `TwoColorMini`, `7color`, `Laser`
- **IR vysílač** (`IRtx`)

---

## 🔌 Nastavení pinů
Nastavení pinů probíhá v připojeném GUI (ESP displej/ Windows aplikace) 
- **Příkaz pro připojení**: `Connect`
- **Příkaz pro odpojení**: `Disconnect`

> **Detailní informace v dokumentaci**

Architektura protokolu, společný model `Device` a příklady komunikace jsou v
[`Dokumentace/VSCP_INTEGRATION_ANALYSIS.md`](Dokumentace/VSCP_INTEGRATION_ANALYSIS.md).

---

## 📦 Požadavky
- **PlatformIO** (VS Code doporučen)
- Deska **ESP32-S3 DevKitC-1**
- USB kabel pro flashování a sériový monitor

Závislosti jsou spravované přes `platformio.ini` (např. Adafruit knihovny, DallasTemperature, ESP32Servo, Encoder, CheapStepper, IRremoteESP8266 aj.).

---

## 🚀 Rychlý start
1. **Otevři projekt** v PlatformIO (kořen s `platformio.ini`).
2. Ujisti se, že je vybráno prostředí:  
   ```ini
   [env:esp32-s3-devkitc-1]
   platform = espressif32
   board    = esp32-s3-devkitc-1
   framework= arduino
