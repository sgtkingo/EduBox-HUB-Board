# EduBox HUB Board – poznámky k vydání

## Bluetooth bridge a VSCP 1.7

- Přidán zabezpečený Bluetooth bridge pro komunikaci s Panelem, včetně párování,
  omezeného rámcování zpráv a diagnostiky spojení.
- Výpadek spojení, BYE a ztráta dohledu ukončují výhradní řídicí relaci a
  převádějí aktuátory do bezpečného stavu. Obnovení vyžaduje nový INIT.
- Opraveno párování a odpojení; zrušené nebo opožděné operace nesmějí obnovit
  neplatnou relaci ani nechtěně zahájit opětovné párování.
- VSCP používá API 1.7 a knihovnu 2.3.0. Aktualizovány emulátor, dokumentace a
  testovací scénáře.

Validace firmware a knihoven se spouští workflow `Bluetooth bridge validation`
na větvi `bluetooth_bridge`.
