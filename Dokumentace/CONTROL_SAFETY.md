# Výhradní řízení Boardu a bezpečné ukončení relace

Politika je v Boardu; formát VSCP zůstává API 1.6. UART, USB a BLE nemohou současně
řídit stejný globální registr fyzických zařízení.

## Vlastník řízení

- První úspěšný INIT získá řízení pro konkrétní transport. Nekompatibilní INIT
  řízení nezíská. Opakovaný úspěšný INIT stejného vlastníka je povolen.
- Druhý transport dostane při INIT chybu `Board busy: another client owns control`.
  Nemůže přemapovat piny, číst ani měnit zařízení, provést RESET nebo vynutit převzetí.
- PING funguje nezávisle na vlastnictví. BYE od jiného transportu neovlivní vlastníka.
- Vlastník uvolní Board pomocí BYE, vypršením dohledu nebo lokálním oznámením
  fyzického odpojení. Nový klient následně musí provést INIT a CONNECT.
- Neúspěšný opakovaný INIT vlastníka odpojí jeho zařízení; řízení nepřevádí jinému klientovi.

## Dohled spojení

V `src/BoardConfig.hpp` jsou nastavení:

- `vscpControlLeaseMs = 10000`: doba bez důkazu aktivity vlastníka před zastavením.
- `vscpControlProbeIntervalMs = 3000`: interval Boardem zahájených PING při nečinnosti.
- `vscpControlProbeTimeoutMs = 500`: timeout odpovědi na jeden PING.

Běžný požadavek vlastníka nebo správná odpověď na Board PING obnoví dohled.
Nečinný klient proto musí obsluhovat příchozí PING: C++ klient pomocí `Client::poll()`.
Panel tuto obsluhu volá jen v platné, neztracené relaci. Po lokálním ukončení
nebo ztrátě relace už nesmí odpovídáním na PING prodlužovat běh výstupů.
Úspěšné předání zápisu nepotvrzuje doručení protistraně.
Board po vypršení dohledu zkusí odeslat BYE, ale vypne výstupy a zruší vlastní
relaci i tehdy, když zápis selže. Obnovení spojení nepřehrává staré CONTROL příkazy.

BLE bridge předává událost ztráty spojení do hlavní smyčky a volá
`deviceRouter.notifyTransportDisconnected(transport)` před novým dispatch a
obsluhou pohybu. Tato cesta zastaví zařízení bez čekání na lease. Router navíc
kontroluje fyzickou dostupnost transportu před vykonáním příkazu. Volání VSCP a
routeru přímo z BLE callbacku není dovoleno. Nepotvrzený CONTROL v Panelu
vyvolá best-effort BYE, lokální uzavření relace a odpojení BLE; příkaz se neopakuje.

## Aktuátory

CONNECT připraví zařízení, ale nezačne pohyb motoru/serva, nezapne laser/LED,
nepíská a nevysílá IR. Aktivace vyžaduje explicitní CONTROL. U DC motoru například:

```text
?type=INIT&api=1.6
?type=CONNECT&id=A02&pins=15
?type=CONTROL&id=A02&speed=50&state=1
?type=BYE&side=client
```

BYE/odpojení zastaví a odpojí všechna připojená zařízení, vymaže jejich stav
CONNECT a seznamy pinů. U aktuátorů se vypne PWM a jejich dosavadní řídicí GPIO
zůstanou OUTPUT/LOW, aby vstupy active-high driverů neplavaly.
Nepoužívá se RESET: ten by mohl servo nebo krokový motor přesouvat do nulové polohy.
Servo ztratí PWM, nikoli napájení; motor ztratí řídicí výstup, nejde o mechanickou brzdu.

Pohyby SG90 a Stepper jsou neblokující a průběžně obsluhované routerem.
Úspěšná odpověď na CONTROL/RESET znamená přijetí pohybu, ne jeho dokončení.
BYE a dohled proto mohou pohyb přerušit. U Stepper změna samotného RPM/DIR
neopakuje poslední pohyb; `angle=0` zruší probíhající pohyb a vypne cívky.

CONNECT odmítne neplatné GPIO, piny komunikačních UART/aktivního USB a kolizi
s jiným připojeným zařízením, pokud se účastní aktuátor. Sdílení pinů mezi
senzory (např. I²C) zůstává možné. A05/A06 nelze připojit současně ani na různé
piny: dosud používají společný globální RGB driver.

## Omezení a hardware

- Toto není certifikovaná ochrana ani náhrada nouzového vypínače.
- Firmware nemůže zaručit okamžitou reakci při blokování jiným ovladačem,
  zamrznutí CPU nebo resetu. Vypršení lease se vyhodnocuje v hlavní smyčce.
- Drivery musí mít odpovídající vypínací úroveň (zde LOW) a externí pull-down,
  aby zůstaly vypnuté také při bootu/resetu a bez připojeného MCU.
- Servo bez PWM nemusí mechanicky držet zatížení. Pro takové aplikace je
  nutná samostatná brzda nebo jiná hardwarová ochrana.
- Vlastnictví transportu není autentizace. BLE bridge navíc vyžaduje Secure
  Connections, MITM, bonding a vazbu na uloženou autentizovanou identitu peeru.
  UART/USB nejsou tímto mechanismem autentizované. Lokální konzole/PIN a BOOT
  vyžadují fyzickou ochranu. Viz [návod bridge v superprojektu](https://github.com/sgtkingo/EduBox-HUB/blob/bluetooth_bridge/docs/BLUETOOTH_BRIDGE.md).

## Testy

```sh
python libraries/engine/tests/run_tests.py
python libraries/vscp/tests/run_tests.py
```

Nativní testy používají simulované GPIO a testují skutečný router a vybrané
ovladače. Nenahrazují měření fyzických výstupů při odpojení, bootu a resetu.
