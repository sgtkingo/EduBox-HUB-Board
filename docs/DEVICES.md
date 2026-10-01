# Zařízení EduBox HUB – zapojení a VSCP parametry

Přehled vychází z registru v `src/main.cpp`, `src/BoardConfig.hpp` a implementací zařízení v `libraries/devices/src`. Platí pro aktuální firmware ESP32-S3-WROOM-1. ID jsou pevná; čísla pinů jsou GPIO, nikoli pozice na konektoru. Výchozí zapojení lze změnit příkazem `CONNECT`.

## Použití a společná pravidla

`CONNECT <ID> <piny>` přiřadí GPIO v pořadí uvedeném v tabulce (více pinů oddělených čárkou). Například `connect S16 15,7,16` znamená X=15, Y=7, SW=16. Poté lze zavolat `UPDATE`, `CONFIG` nebo `CONTROL`; `DISCONNECT` piny uvolní. `UPDATE` vždy vrací také `id` a `status`. Znak „—“ znamená nepodporovanou operaci nebo žádné vlastní klíče; u akčních členů `UPDATE` vrátí `No content`. `CONFIG`/`CONTROL` mohou vrátit `status=1` i při neznámém klíči, protože router neověřuje, zda jej zařízení zpracovalo.

Výchozí terminály: T1=GPIO15, T2=GPIO7, T3=GPIO4, T4=GPIO5. I²C: SDA=GPIO11, SCL=GPIO12. Joystick SW=GPIO16. UART2 používá RX=GPIO18 a TX=GPIO17. Všechna zařízení potřebují společnou GND a napájení odpovídající konkrétnímu modulu. Vstup GPIO ESP32-S3 není 5V tolerantní: 5V výstup senzoru veďte přes převod úrovně. GPIO nesdílejte mezi nezávislými zařízeními; firmware konflikt pinů nehlídá. Výjimkou může být společná I²C sběrnice, ale implementace S09–S11 ji při připojení/odpojení inicializuje a ukončuje, takže souběh těchto UID nemusí fungovat spolehlivě.

## Senzory

| ID | Name | Description | Piny – kam zapojit (`CONNECT`) | Values – `UPDATE` | `CONFIG` | `CONTROL` |
| --- | --- | --- | --- | --- | --- | --- |
| S00 | DS18B20 | OneWire teploměr | DATA→volný GPIO, např. 5 (`S00 5`); externí pull-up cca 4,7 kΩ k 3,3 V | `temp` °C; `alarm` OK/LOW/HIGH | `res` 9–12 bitů; `lalarm`/`lowalarm` a `halarm`/`highalarm` °C | — |
| S01 | DHT11 | Teplota a vlhkost | DATA→7 (`S01 7`) | `temp` °C/°F; `humi` %; při chybě `nan` | `unit=F` pro °F, jinak °C; `hi=true` heat index | — |
| S02 | Dhall | Digitální Hall | DO→7 (`S02 7`) | `state` 0/1 | — | — |
| S03 | Ahall | Analogový Hall | AO→7 (`S03 7`) | `val` ADC; `polarity` NORTH/SOUTH/NO MAGNET | `res` bitů; `llimit`, `hlimit` prahy ADC | — |
| S04 | PInterrupt | Digitální přerušovací modul | DO→15 (`S04 15`) | `state` 0/1 | — | — |
| S05 | FC51 | IR překážkový modul | DO→15 (`S05 15`) | `state` 0/1 | — | — |
| S06 | HCSR04 | Ultrazvukový dálkoměr | TRIG→15, ECHO→7 (`S06 15,7`); 5V ECHO přes převod na 3,3 V | `distance` cm | `limit` maximální dosah cm; `delay` prodleva ms | — |
| S07 | HCSR501 | PIR pohyb | OUT→15 (`S07 15`) | `state` 0/1 | — | — |
| S08 | KW113Z | Digitální snímač KW113Z | DO→15 (`S08 15`) | `state` 0/1 | — | — |
| S09 | BMP280 | Teplota a tlak, I²C 0x76 | SDA→11, SCL→12 (`S09 11,12`) | `temp` °C; `press` hPa | `os_temp`, `os_press` převzorkování; `filter` | — |
| S10 | BMP180 | Tlak a výška, I²C 0x77 | SDA→11, SCL→12 (`S10 11,12`) | `press` hPa; `altitude` m | `gain` násobek tlaku | — |
| S11 | TCS34725 | Barevný senzor, I²C 0x29 | SDA→11, SCL→12 (`S11 11,12`) | `R`, `G`, `B` 0–255 (velká písmena) | `itime` ms: 2/50/101/199/300/401/499/600; `gain` 1/4/16/60× | — |
| S12 | IRrx | IR přijímač | OUT→15 (`S12 15`) | `code` hex `0xXXXXXXXX`; před příjmem 0 | `dedup` potlačení opakování ms | — |
| S13 | Dntc | Digitální NTC modul | DO→15 (`S13 15`) | `state` 0/1 | — | — |
| S14 | Antc | Analogový NTC teploměr | AO→7 (`S14 7`) | `temp` °C, závisí na odporovém děliči | `res` bitů; `filter` 0 bez / 1 lehký / 2 silný | — |
| S15 | PHresistance | Fotorezistor | AO/dělič→7 (`S15 7`) | `intensity` přibližné lux | `res` bitů; `gain` násobek | — |
| S16 | Joystick | Analogové X/Y a tlačítko | X→15, Y→7, SW→16 (`S16 15,7,16`) | `direction` CLICK/CENTER/LEFT/RIGHT/UP/DOWN | `res` bitů; `threshold` mrtvá zóna % | — |
| S17 | HallLin | Lineární Hall | AO→7 (`S17 7`) | podle `unit` klíč `ADC`, `Voltage` (V) nebo `Induction` (mT) | `res` bitů; `unit=ADC` / `Voltage` / `Induction` | — |
| S18 | MQ135 | Digitální výstup plynového modulu | DO→15 (`S18 15`); prověřit úroveň DO | `state` 0/1 | — | — |
| S19 | DMoisture | Digitální vlhkost | DO→15 (`S19 15`) | `state` 0/1 | — | — |
| S20 | TTP223 | Kapacitní dotyk | OUT→15 (`S20 15`) | `state` 0/1 | — | — |
| S21 | GP2Y0A21YK0F | Sharp IR vzdálenost | AO→15 (`S21 15`) | `distance` cm/mm; `alarm` OK/LOW/HIGH | `unit=0` cm / `unit=1` mm; `lowalarm`, `highalarm` | — |
| S22 | Rencoder | Kvadraturní enkodér | **B→4, A→5** (`S22 4,5`); pořadí určuje `attach()` | `position` kroky; `alarm` OK/LOW/HIGH | `direction=reverse` (jinak normální); `lowalarm`, `highalarm` | — |
| S23 | HS0038DB | Digitální IR přijímač | OUT→15 (`S23 15`) | `state` 0/1 | — | — |
| S24 | TCRT5000 | Odrazový IR senzor | DO→15 (`S24 15`) | `state` 0/1 | — | — |
| S25 | IRflame | IR detektor plamene | DO→15 (`S25 15`) | `state` 0/1 | — | — |
| S26 | REED | Jazýčkový kontakt | DO→7 (`S26 7`) | `state` 0/1 | — | — |
| S27 | MicSmall | Malý analogový mikrofon | AO→15 (`S27 15`) | `volume` relativní logaritmická hodnota, ne kalibrované dB SPL | `res` bitů; `time` vzorkování ms | — |
| S28 | MicBig | Velký analogový mikrofon | AO→15 (`S28 15`) | `volume` relativní logaritmická hodnota, ne kalibrované dB SPL | `res` bitů; `time` vzorkování ms | — |
| S29 | MetalTouch | Digitální kovový dotyk | DO→15 (`S29 15`) | `state` 0/1 | — | — |
| S30 | Heartbeat | Analogový snímač pulzu | AO→7 (`S30 7`) | `bpm` tepů/min; měření cca 5 s | — | — |
| S31 | Btn | Tlačítko/modul | DO→7 (`S31 7`) | `state` 0/1 | — | — |
| S32 | TiltSwitch | Náklonový spínač | DO→7 (`S32 7`) | `state` 0/1 | — | — |
| S33 | Dvibration | Digitální vibrace | DO→7 (`S33 7`) | `state` 0/1 | — | — |
| S34 | HGswitch | Digitální spínač HGswitch | DO→7 (`S34 7`) | `state` 0/1 | — | — |
| S35 | Tap | Digitální klepnutí | DO→7 (`S35 7`) | `state` 0/1 | — | — |

U digitálních modulů `state` znamená pouze úroveň GPIO; polarita „detekováno“ závisí na modulu. S00 vyžaduje čidlo už při `CONNECT`. S06 při inicializaci vyžaduje echo od objektu dále než 2 cm; S21 naměřenou vzdálenost 20–80 cm, jinak `CONNECT` selže. S16 kalibruje střed při prvním `UPDATE`, držte páčku uprostřed. Pro S30 nastavte timeout emulátoru alespoň 7 s.

## Akční členy

| ID | Name | Description | Piny – kam zapojit (`CONNECT`) | Values – `UPDATE` | `CONFIG` | `CONTROL` |
| --- | --- | --- | --- | --- | --- | --- |
| A00 | SG90 | Servo | SIGNAL→15 (`A00 15`); externí napájení, společná GND | — | — | `angle` 0–180°; `speed` 0–100 (vyšší = rychlejší) |
| A01 | Stepper | Krokový motor přes driver | IN1→15, IN2→7, IN3→4, IN4→5 (`A01 15,7,4,5`) | — | — | `angle` stupně; `dir=true/1` vpřed, jinak zpět; `rpm` |
| A02 | DC | DC motor přes driver/PWM | PWM vstup driveru→15 (`A02 15`) | — | — | `speed` 0–100 %; `state=true/1/on` zapnuto, jinak vypnuto |
| A03 | TwoColor | Dvoubarevná LED | R→4, G→5 (`A03 4,5`); odpory | — | — | `color=R/G`; `brig` 0–100 % |
| A04 | TwoColorMini | Malá dvoubarevná LED | R→5, G→4 (`A04 5,4`); opačně než A03 | — | — | `color=R/G`; `brig` 0–100 % |
| A05 | RGB | RGB LED | R→15, G→7, B→4 (`A05 15,7,4`); odpory | — | — | `brigr`, `brigg`, `brigb` 0–100 % |
| A06 | RGB | Druhý záznam RGB LED | R→15, G→7, B→4 (`A06 15,7,4`); piny lze změnit | — | — | `brigr`, `brigg`, `brigb` 0–100 % |
| A07 | Color7 | Sedmibarevný LED modul | Řídicí vstup→15 (`A07 15`) | — | — | `control=true/1/on` zapnuto, jinak vypnuto |
| A08 | IRtx | IR vysílač | IR LED/driver→7 (`A08 7`); proudový odpor | — | — | `code` 32bitový hex, např. `0x00FF18E7` |
| A09 | Laser | Laserový modul | Řídicí vstup→7 (`A09 7`) | — | — | `control=true/1/on` zapnuto, jinak vypnuto |
| A10 | BuzzP | Pasivní bzučák | SIGNAL→15 (`A10 15`) | — | — | `freq` Hz; `duration` ms |
| A11 | BuzzA | Aktivní bzučák | SIGNAL→15 (`A11 15`) | — | — | `control=true/1/on` zapnuto, jinak vypnuto |

Motory ani servo výkonově nepřipojujte přímo k GPIO. `CONNECT` nechává aktuátory vypnuté; spuštění vyžaduje explicitní `CONTROL` (u A02 například `speed=50&state=1`). Servo a krokový motor přijímají pohyb neblokujícím způsobem. Laserem nemiřte do očí. A05/A06 používají společný globální RGB driver a Board jejich současné připojení odmítá. Kolize GPIO s aktuátory a komunikačními piny se kontrolují. Pravidla výhradního řízení, dohledu spojení a zastavení při `BYE` jsou v [CONTROL_SAFETY.md](CONTROL_SAFETY.md).

## Příklad v emulátoru

    init
    connect S01 7
    update S01
    config S01 unit=F hi=true
    update S01
    disconnect S01
    connect A05 15,7,4
    control A05 brigr=100 brigg=0 brigb=25
    disconnect A05

Před přepojením vodičů nebo použitím stejného GPIO pro jiný modul zavolejte `DISCONNECT`.
