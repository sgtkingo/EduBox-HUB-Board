# VSCP architektura EduBox HUB FW

## Výsledek refactoringu

Firmware používá společný **Virtual Sensors Communication Protocol** API `1.4`
pro klienta i server. Implementace vychází z
[`sgtkingo/VSCP`](https://github.com/sgtkingo/VSCP), commit
`0d4b02f03802966ba4497bc603d76b5d32922ead`, ale původní globální klient a UART
messenger jsou rozdělené na samostatné vrstvy.

Zkratka VSCP zde neoznačuje eventový Very Simple Control Protocol z vscp.org.

## Knihovna `libraries/vscp`

### Společná vrstva

- `vscp_types.hpp` definuje API `1.4`, příkazy, stav a datové struktury.
- `vscp_codec.hpp` parsuje a serializuje řádkové zprávy
  `?key=value&key2=value2`.
- `vscp_transport.hpp` odděluje protokol od konkrétní komunikační linky.
- `vscp_stream_transport.hpp` implementuje neblokující framing nad Arduino
  `Stream`, limit 1024 B a vždy řádkově ukončený výstup.

### Klient

`vscp_client.hpp` implementuje synchronní HMI/controller API:

```cpp
vscp::Client client(transport);
client.init("board", "1.0");
client.connect("S01", "7");
vscp::ResponseStatus values = client.update("S01");
```

Klient kontroluje inicializaci, timeout, `status` a shodu odpovědního `id`.

### Server

`vscp_server.hpp` implementuje neblokující responder. Příkazy směruje přes
samostatné handlery a pro každý přidaný transport udržuje vlastní stav `INIT`.

```cpp
server.on(vscp::Command::Config, configHandler);
server.on(vscp::Command::Control, controlHandler);
server.addTransport(uartTransport);
server.poll();
```

Server podporuje `INIT`, `CONNECT`, `DISCONNECT`, `UPDATE`, `CONFIG`, `CONTROL`
a `RESET`. `CONFIG` a `CONTROL` sdílejí wire formát, ale jsou dvě samostatné
logické operace.

## Jednotný model zařízení

`src/Device.hpp` nahrazuje rozdílné základní kontrakty senzoru a aktuátoru.
Každé zařízení má:

- `DeviceType deviceType()` pro jednoznačný typ zařízení;
- `init()`, `reset()`, `attach()` a `detach()` pro lifecycle;
- `update()` pro čtení runtime hodnot;
- `config()` pro vnitřní konfiguraci;
- `control()` pro ovládání runtime výstupu.

Výchozí implementace nepodporované operace ignoruje. Konkrétní zařízení
přepisuje pouze operace, které skutečně používá. `Sensor.hpp` a `Actuator.hpp`
jsou dočasné kompatibilní aliasy na `Device`; všechny objekty jsou v runtime
uložené společně jako `RegisteredDevice`.

`DeviceType` rozlišuje konkrétní hardware, například `Dht11`, `Bmp280`,
`ServoSg90`, `DcMotor`, `RgbLed` nebo `PassiveBuzzer`. Prefix UID `Sxx`/`Axx`
zůstává zachovaný kvůli kompatibilitě databáze a HMI, ale už neurčuje dostupné
C++ metody.

## Firmware router

`src/VscpDeviceRouter.cpp` propojuje generický server s hardwarem:

| Příkaz | Handler |
| --- | --- |
| `INIT` | Kontrola API `1.4`. |
| `CONNECT` | Vyhledání UID, parsování pinů, `attach()` a `init()`. |
| `DISCONNECT` | `detach()` a vyčištění connection state. |
| `UPDATE` | `Device::update()` a převod hodnot do response. |
| `CONFIG` | Samostatné volání `Device::config()`. |
| `CONTROL` | Samostatné volání `Device::control()`. |
| `RESET` | Reset jednoho UID nebo wildcard `S*`/`A*`. |

Router už neparsuje index zařízení a nepřistupuje do oddělených polí senzorů a
aktuátorů. Vyhledává přímo UID ve společném registru.

## Bootstrap a transporty

`src/main.cpp` obsahuje jediný registr `registeredDevices` pro `S00..S35` a
`A00..A11`. Server je dostupný na:

- USB `Serial`, 115200 baudů;
- UART2, 115200 8N1, RX 18 a TX 17.

Oba transporty používají stejný codec a stejné handlery, ale samostatný stav
inicializace.

## Příklad komunikace

```text
> ?type=INIT&app=board&db=1.0&api=1.4
< ?status=1

> ?type=CONNECT&id=S01&pins=7
< ?id=S01&status=1

> ?type=CONFIG&id=S01&unit=F&hi=true
< ?id=S01&status=1

> ?type=UPDATE&id=S01
< ?humi=...&id=S01&status=1&temp=...

> ?type=CONNECT&id=A00&pins=15
< ?id=A00&status=1

> ?type=CONTROL&id=A00&angle=90&speed=50
< ?id=A00&status=1
```

Pořadí parametrů response není významové. Klient musí hodnoty parsovat podle
klíčů.

## DS18B20

`S00` už nemá paralelní implementaci ve glue vrstvě. `DS18B20` vlastní dynamické
instance `OneWire` a `DallasTemperature`, které vytváří podle pinu z `CONNECT`.
Konfigurace akceptuje původní `lalarm`/`halarm` i čitelnější
`lowalarm`/`highalarm`.

## Původ a distribuce

Podrobnosti o převzetí jsou v `libraries/vscp/UPSTREAM.md`. Referenční commit
neobsahoval licenční soubor. Před distribucí vendorizované knihovny mimo projekt
je nutné licenční podmínky vyjasnit s vlastníkem upstream repozitáře.

## Ověření

Projekt je sestavitelný příkazem:

```powershell
C:\Users\jirka\.platformio\penv\Scripts\platformio.exe run
```

Ověřený build environment: `nodemcu-32s`, framework Arduino, knihovna `vscp`
`2.0.0` nalezená přes `lib_extra_dirs = libraries`.
