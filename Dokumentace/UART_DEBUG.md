# UART diagnostika

Debugger vypisuje na UART0 (USB-UART konektor, ne nativní USB CDC) rychlostí 115200 baud:

- `LOG [čas_ms][RX] USB|UART2|BT ...` – kompletní požadavek přijatý z daného transportu.
- `LOG [čas_ms][TX] USB|UART2|BT ...` – odpověď odeslaná přes daný transport.
- `LOG [čas_ms][DEBUG] ...` – průběh inicializace Boardu, BT bridge a workflow `INIT`.
- `LOG [čas_ms][ERROR] ...` – chyba rámce/zápisu, BT bridge nebo VSCP odpověď se `status=0`; řádek obsahuje transport, ID a důvod, pokud jsou dostupné.
- `LOG [čas_ms][WARN] ...` – neplatná hodnota senzoru (`nan`, `inf`, `-inf`).

V `src/BoardConfig.hpp` nastavte `uartDebugEnabled=false` pro vypnutí aplikační diagnostiky, nebo `uartDebugTraceEnabled=false` pro vypnutí pouze RX/TX. `usbProtocolEnabled=false` vyhradí UART0 pro diagnostiku a vypne jeho VSCP server; UART podle `vscpUartConfig` zůstane aktivní. Ve výchozím stavu zůstává na UART0 i protokol, aby fungoval Python emulátor (řádky nezačínající `?` ignoruje).

`CORE_DEBUG_LEVEL=2` v `platformio.ini` povoluje také chyby a varování Arduino/ESP. Ty mohou mít vlastní formát a nejsou všechny řízené aplikačním přepínačem. Boot/panic výpisy ROM a SDK nejsou tímto debuggerem filtrovány. Diagnostika je textový logger, nikoli breakpointový debugger. Neznámé CONFIG/CONTROL klíče zařízení zatím nevalidují, takže pro ně nelze slíbit varování.

Otevřete Serial Monitor na USB-UART portu. RX/TX jsou z pohledu Boardu. Při velmi častém provozu mohou kompletní výpisy zpomalit obsluhu; vypněte tehdy trace. Výpisy mohou obsahovat citlivé parametry požadavků.

## Python monitor

Nástroj je v `Apps/emulator/monitor.py`, používá stejné `requirements.txt` jako emulátor:

    cd Apps/emulator
    python -m pip install -r requirements.txt
    python monitor.py --ports
    python monitor.py --port COM4
    python monitor.py --port COM4 --levels ERROR,WARN
    python monitor.py --port COM4 --levels RX,TX --contains S01
    python monitor.py --port COM4 --timestamps --log monitor.log

Bez `--port` se automaticky vybere jediný dostupný sériový port; při více portech je nutný explicitní výběr. Vyberte USB-UART0 port Boardu, ne port panelu či nativního USB. Výchozí rychlost je 115200 8N1, změna přes `--baud`.

Monitor nic neodesílá. Zobrazuje i systémové chyby, warningy a boot/backtrace text; při použití `--levels` se zobrazí pouze vybrané kategorie (ERROR, WARN, INFO, DEBUG, RX, TX, PROTOCOL, OTHER). Doprovodné řádky backtrace zpravidla patří do OTHER. `--log` připojuje pouze zobrazené řádky do UTF-8 souboru, nic nepřepisuje. `--timestamps` přidá čas počítače, zatímco čas uvnitř LOG je doba běhu Boardu.

Ukončení: Ctrl+C. Před spuštěním zavřete PlatformIO Serial Monitor i emulátor na stejném COM portu. Pro sledování komunikace panel–Board otevřete monitor na UART0 a panel připojte k UART2. Monitor před otevřením deaktivuje DTR/RTS a nemaže vstupní buffer, ale některé převodníky/ovladače přesto mohou při otevření krátce změnit signály a resetovat ESP. Neplatné UTF-8 znaky jsou nahrazeny; řádky delší než 16 KiB se zkracují.
