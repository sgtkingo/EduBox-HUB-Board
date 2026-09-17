# EduBox HUB Board

![Logo EduBox HUB Board](assets/logo.svg)

**EduBox HUB Board** je hardwarová a firmwarová část ekosystému
[EduBox HUB](https://github.com/sgtkingo/EduBox-HUB). Centrální jednotka
zajišťuje připojení senzorů a aktuátorů a zpřístupňuje je ostatním částem
ekosystému prostřednictvím protokolu
[EduBox HUB VSCP](https://github.com/sgtkingo/EduBox-HUB-VSCP).

## Zařazení v ekosystému

```text
EduBox HUB
├── Board  ← tento repozitář
├── App
├── Panel
│   └── Firmupdater
└── VSCP
```

Firmware je určen pro **ESP32-S3 DevKitC-1** a slouží ke čtení dat ze
senzorů, ovládání aktuátorů a komunikaci s
[EduBox HUB App](https://github.com/sgtkingo/EduBox-HUB-App) nebo
[EduBox HUB Panel](https://github.com/sgtkingo/EduBox-HUB-Panel).

[Dokumentace senzorů](Dokumentace/Senzory_dokumentace.pdf)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/) 
[![Board](https://img.shields.io/badge/Board-ESP32--S3--DevKitC--1-blue)](#požadavky) 
[![Framework](https://img.shields.io/badge/Framework-Arduino-green)](https://docs.platformio.org/en/latest/frameworks/arduino.html)

---

## Struktura projektu

- `src/` obsahuje pouze firmware bootstrap, konfiguraci desky a setup vstupy.
- `libraries/vscp/` obsahuje multiplatformní komunikační protokol.
- `libraries/engine/` obsahuje společný `Device` model a VSCP router.
- `libraries/devices/` obsahuje konkrétní senzory a aktuátory.
- `lib/` zůstává pouze pro starší lokální ovladače třetích stran.

---

## ✨ Funkce
- Čtení mnoha analogových i digitálních senzorů (teplota, vlhkost, tlak, barva, vzdálenost, zvuk, IR, …).
- Ovládání aktuátorů (servo, DC motor, krokový motor, bzučáky, RGB LED, laser, IR vysílač).
- Komunikace přes **VSCP API 1.5** (USB Serial a UART2) – společný klientský codec a handler-based server.
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

> Kompletní seznam je v `libraries/devices/src/`; globální registr zařízení
> zůstává v `src/main.cpp`.

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
