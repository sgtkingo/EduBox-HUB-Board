# EduBox VSCP klientský emulátor

Python nástroj emuluje klienta komunikujícího s firmwarem EduBox HUB po USB
Serial nebo UART2. Podporuje příkazy `INIT`, `CONNECT`, `DISCONNECT`, `UPDATE`,
`CONFIG` a `CONTROL` protokolu VSCP API 1.4.

## Instalace

```powershell
cd Apps\emulator
py -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
```

Před spuštěním zavřete PlatformIO Serial Monitor, protože jeden COM port nemůže
být ve Windows otevřený dvěma programy současně.

## Připojení k firmware

Firmware nabízí protokol na těchto dvou rozhraních:

- nativní USB CDC konektor ESP32-S3 (`Serial`);
- UART2, `115200 8N1`, GPIO 18 = RX a GPIO 17 = TX.

Vestavěný převodník Silicon Labs CP210x na DevKitC je obvykle připojený k
programovacímu UART0 a firmware na něm protokol neposlouchá. Použijte druhý,
nativní USB konektor desky, nebo externí USB/UART převodník zapojený křížem:
TX převodníku na GPIO 18, RX převodníku na GPIO 17 a společnou GND.

## Použití

Výpis dostupných portů:

```powershell
.venv\Scripts\python emulator.py ports
```

Výpis obsahuje také popis adaptéru, aby šlo odlišit nativní USB od CP210x.

Automatický scénář všech příkazů:

```powershell
.venv\Scripts\python emulator.py --port COM4 scenario
```

Výchozí scénář nakonfiguruje a přečte analogový senzor `S03` na pinu 7, odpojí
jej, připojí aktuátor `A09` na stejném pinu a příkazem `CONTROL` jej ponechá
vypnutý. Zařízení a piny lze změnit:

```powershell
.venv\Scripts\python emulator.py --port COM4 scenario `
  --sensor-id S09 --sensor-pins 11,12 --config os_temp=2 `
  --actuator-id A00 --actuator-pins 15 --control angle=45
```

Interaktivní režim:

```powershell
.venv\Scripts\python emulator.py --port COM4 shell
```

Příklad interaktivní relace:

```text
vscp> init
vscp> connect S03 7
vscp> config S03 res=12 llimit=1700 hlimit=2100
vscp> update S03
vscp> disconnect S03
vscp> connect A09 7
vscp> control A09 control=off
vscp> disconnect A09
```

Jeden samostatný požadavek lze poslat pomocí `send`:

```powershell
.venv\Scripts\python emulator.py --port COM4 send INIT api=1.4 app=test db=1.3
```

Emulátor vypisuje přesný odeslaný (`TX`) a přijatý (`RX`) rámec. Firmware
vyžaduje inicializaci pro všechny příkazy kromě `INIT` a `CONNECT`; samostatná
spuštění `send` proto nemají sdílený stav relace.
