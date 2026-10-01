# Release workflow Boardu

Board používá stejný čtyřčíselný formát verze a commit markery jako Panel.
Verze je uložena v kořenovém souboru [`VERSION`](../../VERSION) ve formátu
`MAJOR.MINOR.PATCH.BUILD`. Aktuální počáteční hodnota je `1.0.0.0`.

## Vytvoření release

1. Aktualizujte [`RELEASE_NOTES.md`](../../RELEASE_NOTES.md); nejnovější sekce
   má být nahoře a měla by obsahovat cílovou verzi a datum vydání.
2. Připravte poslední commit na `main` s právě jedním markerem na začátku
   předmětu:

   | Marker | Změna verze |
   | --- | --- |
   | `(major)` | `MAJOR + 1`, ostatní části na nulu |
   | `(minor)` | `MINOR + 1`, patch/build na nulu |
   | `(patch)` | `PATCH + 1`, build na nulu |
   | `(build)` | `BUILD + 1` |

   Například `(patch) Oprava čtení senzoru`.
3. Pushněte commit na `main`. `Release Auto` zvýší `VERSION`, vytvoří release
   commit, sestaví všechna prostředí z `platformio.ini`, aktualizuje binární
   soubory, vytvoří tag `vMAJOR.MINOR.PATCH.BUILD` a GitHub Release.

Commity bez markeru release nevytvoří. Automatické commity začínají
`chore(release):` a release gate je přeskočí. Workflow lze spustit také ručně
z větve `main`; ruční spuštění používá stejný marker gate. Po chybě znovu spusťte pouze
neúspěšné joby, aby se už provedený bump verze neaplikoval podruhé.

## Cílová prostředí a artefakty

Build workflow samostatně sestavuje tyto PlatformIO environmenty:

- `esp32-s3-devkitc-1`
- `esp32-s3-devkitc-2`
- `nodemcu-32s`

Výstupy každého prostředí jsou v `bin/<environment>/latest/`; bezprostředně
před aktualizací se předchozí soubory přesunou do `bin/<environment>/backup/`.
`backup` uchovává jednu předchozí sadu. Verze vydané dříve zůstávají dostupné
v tagu a GitHub Release. Souborové názvy uvnitř složek odpovídají PlatformIO
výstupům (například `firmware.bin`, `bootloader.bin` a `partitions.bin`).

GitHub Release obsahuje soubory se jménem `<environment>-<soubor>`, aby se
stejnojmenné výstupy různých desek nepřepsaly. Workflow ověřuje přítomnost
`firmware.bin`, `bootloader.bin` a `partitions.bin` pro každý target; přiloží
všechny `.bin` soubory vytvořené příslušným buildem.

## Samostatný build bez release

Workflow `Build Board Firmware` lze ručně spustit z GitHub Actions. Volitelně
přijímá Git ref a název artefaktů. Vytvoří dočasné Actions artefakty pro každý
target, ale nemění `VERSION` ani složku `bin` a nevytváří tag ani release.

## Workflow soubory

- [`build.yml`](../../.github/workflows/build.yml) – opakovaně použitelný i
  ručně spustitelný PlatformIO build.
- [`release-auto.yml`](../../.github/workflows/release-auto.yml) – commit gate,
  bump verze, build, aktualizace `bin`, tag a GitHub Release.

Při přidání nebo přejmenování prostředí v `platformio.ini` aktualizujte také
matici prostředí v obou workflow a download kroky v `release-auto.yml`.
