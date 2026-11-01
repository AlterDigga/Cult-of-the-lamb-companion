# Cult of the Lamb Companion – Umsetzungsplan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ein Eden-Duo-Companion `01002E7016C46000.dsmod.zip` für Cult of the Lamb 1.5.27 mit den Tabs
Kult, Probleme, Follower, Inventar und Dungeon auf dem zweiten Bildschirm des AYN Thor.

**Architecture:** Das Manifest (generiert von `tools/gen_manifest.py`) beschreibt Seiten und Widgets.
Einfache Skalare liest die Per-Build-Datei über IL2CPP-Pointerketten. Ein natives C++-Modul liest
Listen (Follower, Quests, Inventar, Räume), leitet Probleme ab, liefert englische Texte aus
I2-Localization sowie Icons aus Unity-Assets und veröffentlicht alles als Werte `name{i}` für
`repeat` + `scroll`.

**Tech Stack:** C++20, CMake + Ninja, Android NDK r28c (arm64, `c++_static`), Eden Duo v1.3.2.2
(SDK-Header, `eden-cli`), `ce_unity` aus `eden-duo-companions` (Unity SerializedFile/Bundle/Sprite),
Python 3 (nur Standardbibliothek) für Generatoren, WSL2 Ubuntu 24.04.

**Spec:** `docs/superpowers/specs/2026-10-31-cotl-companion-design.md`, Offsets in `docs/recon-1.5.27.md`.

## Global Constraints

- Title-ID `01002E7016C46000`; Build-ID genau `316A4D4881A9F1E077A07262E917917C` + 32 Nullen; Datendatei `316A4D4881A9F1E0.json`.
- Eden Duo **v1.3.2.2** (Tag `v1.3.2.2`) für SDK und `eden-cli`; Modul-ABI 1; `min_runtime` = höchste genutzte Runtime (erwartet 22).
- Canvas 1240 × 1080.
- Nur lesen: kein `write`, `slot_write`, `call`, `sequence`, keine `patches`, keine `write_memory`-Nutzung.
- Paket enthält keine Spielinhalte (keine Texte, Bilder, Daten des Spiels). Enum-Namen aus dem IL2CPP-Dump sind Code-Bezeichner und erlaubt.
- Texte nur Englisch.
- Karte zeigt nur Räume mit `Discovered` oder `Visited`.
- Keine Heuristik: jede Schwelle und Regel wird aus Spielcode/-daten abgeleitet und im Code mit Quelle (Methode + Offset aus `dump.cs`) kommentiert.
- Jeder Speicherzugriff über `is_mapped` + `read_memory`; Listen auf `_size ≤ 512` begrenzt; doppelt lesen und vergleichen (Torn-Read-Schutz), sonst `*.ready = 0` veröffentlichen.
- Modul-Callbacks fangen alle Exceptions.
- IL2CPP v31: `Il2CppClass.static_fields` @ +0xB8; `List<T>`: `_items` @ 0x10, `_size` @ 0x18; Array: `max_length` @ 0x18, Elemente ab 0x20; `String`: `length` @ 0x10 (s32), UTF-16 ab 0x14.
- `game/`, `work/`, `tools/hactool`, `tools/il2cppdumper` nie committen (`.gitignore`).
- Commits enden mit `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Review Focus

1. **Laden, Menüs, Titelbild:** `DataManager.instance` oder `PlayerFarming.Instance` ist null bzw. halb initialisiert → Companion zeigt „Waiting for game…“, nie Nullwerte oder Absturz.
2. **Sehr viele Follower (> 40) und leere Listen (0 Follower, 0 Probleme, leeres Inventar):** Scrollen funktioniert, leere Zustände zeigen einen Hinweistext statt einer leeren Fläche.
3. **Run endet durch Tod oder Rückkehr, neuer Run direkt danach:** Rücksprung auf den vorherigen Tab genau einmal; kein Hängenbleiben auf dem Dungeon-Tab; Karte des alten Runs verschwindet.
4. **Unbekannte Enum-Werte (DLC-Items, neue Traits) oder fehlender I2-Begriff:** Anzeige des Enum-Namens bzw. eines neutralen Platzhalter-Icons, kein leerer Eintrag, kein Crash.
5. **Speicherstand mit zwei Spielern (Koop) oder Follower als Missionar/Gefangener/tot während der Anzeige:** nur Spieler 1; Follower wechselt sauber zwischen Gruppen ohne Dopplung.

Die zugehörigen Tests stehen in Task 3 (1), Task 4/6 (2), Task 8 (3), Task 5/7 (4), Task 4/8 (5).

---

## Dateistruktur

```
cotl-companion/
  CMakeLists.txt                         Standalone-Build (Modul + Tests)
  native/
    module.cpp                           ABI-Einstieg, create/sample/tick/destroy, load_image
    guest.h / guest.cpp                  Guest-Interface, HostGuest, IL2CPP-Helfer (Ptr, List, String)
    offsets_1_5_27.h                     Alle Offsets/TypeInfo-Adressen als constexpr
    snapshot.h / snapshot.cpp            Spielzustand lesen → GameSnapshot (Torn-Read-Schutz)
    problems.h / problems.cpp            Probleme + Quests ableiten und sortieren (rein, testbar)
    publish.h / publish.cpp              GameSnapshot/Probleme → veröffentlichte Werte
    i2.h / i2.cpp                        I2Languages aus resources.assets, englische Spalte
    terms.h / terms.cpp                  Enum-ID → Enum-Name → I2-Begriff
    enums_1_5_27.inc                     generiert: Enum-Namen (TraitType, ITEM_TYPE, Card, EquipmentType, FollowerRole, Thought)
    icons.h / icons.cpp                  module:cotl:… Bildschlüssel → RGBA über ce_unity
    dungeon_map.h / dungeon_map.cpp      Räume → Kartenzellen (nur entdeckt)
  tests/
    test_util.h                          CHECK/CHECK_EQ-Makros, FakeGuest
    guest_test.cpp, snapshot_test.cpp, problems_test.cpp, publish_test.cpp,
    i2_test.cpp, terms_test.cpp, dungeon_map_test.cpp
  package/CultOfTheLamb/
    package.json
    dualscreen/manifest.json             generiert, nicht von Hand bearbeiten
    dualscreen/316A4D4881A9F1E0.json     Points (Skalare)
  tools/
    setup_wsl.sh                         Abhängigkeiten, Eden Duo, NDK
    gen_enums.py                         dump.cs → native/enums_1_5_27.inc
    gen_manifest.py                      erzeugt dualscreen/manifest.json
    build.sh                             Linux + Android bauen, strippen, Paket bauen
    headless.sh                          eden-cli --aux-virtual mit Spielstand, Screenshots
  docs/re-notes.md                       RE-Funde mit Quelle (Methode, Offset, Verifikation)
```

Abhängigkeiten außerhalb des Repos (per `tools/setup_wsl.sh` unter `~/cotl-deps/`):
`eden-duo` @ v1.3.2.2 (mit `.cache/cpm`), `eden-duo-companions` @ main (für `ce_unity` und
`tools/build_dualscreen_package.py`), Android NDK r28c.

## Namensschema veröffentlichter Werte

| Präfix | Werte |
|---|---|
| `g.` | `g.ready`, `g.in_run`, `g.day`, `g.cult_level` |
| `cult.` | `cult.faith`, `cult.hunger`, `cult.illness`, `cult.illness_max`, `cult.alive`, `cult.old`, `cult.sick`, `cult.storage_pct` |
| `fol.` | `fol.count`, `fol.name{i}`, `fol.level{i}`, `fol.role{i}` (Text), `fol.sat{i}`, `fol.faith{i}`, `fol.ill{i}`, `fol.happy{i}` (0–100), `fol.group{i}` (0 normal, 1 Missionar, 2 Gefangener), `fol.detail{i}` (Text) |
| `prob.` | `prob.count`, `prob.n_<kind>` je Art, `prob.name{i}`, `prob.level{i}`, `prob.kind{i}` (int), `prob.reason{i}` (Text) |
| `inv.` | `inv.count`, `inv.name{i}`, `inv.qty{i}`, `inv.icon{i}` (`module:cotl:item/<ITEM_TYPE>`), `inv.quest{i}` (0/1) |
| `run.` | `run.hp_red`, `run.hp_red_max`, `run.hp_blue`, `run.hp_black`, `run.hp_spirit`, `run.hp_spirit_max`, `run.hp_fire`, `run.hp_ice`, `run.weapon`, `run.weapon_lvl`, `run.curse`, `run.curse_lvl`, `run.relic`, `run.tarot_count`, `run.tarot_name{i}`, `run.tarot_lvl{i}`, `run.tarot_text{i}`, `run.tarot_icon{i}` |
| `map.` | `map.count`, `map.x{i}`, `map.y{i}` (Canvas-px), `map.state{i}` (1 gesehen, 2 besucht, 3 aktuell), `map.kind{i}` (0 normal, 1 Boss, 2 Shop, 3 Respawn, 4 sonstige Sonderräume), `map.link_count`, `map.lx{i}`, `map.ly{i}`, `map.lw{i}`, `map.lh{i}` |

`ProblemKind` (int, Sortierreihenfolge): 0 `Starving`, 1 `Ill`, 2 `Injured`, 3 `Dissent`, 4 `QuestAccepted`, 5 `QuestOffered`, 6 `Exhausted`, 7 `Cursed`, 8 `Old`, 9 `Hungry`.
Die genaue Reihenfolge zwischen Gruppen gibt die Spec vor (lebensbedrohlich → Dissens → Quests → Alter);
`Hungry`/`Exhausted`/`Cursed` sortiert Task 6 nach ihrer in `docs/re-notes.md` belegten Dringlichkeit ein,
und diese Tabelle wird dort aktualisiert.

---

### Task 1: Umgebung, Repo und Headless-Lauf

**Files:**
- Create: `tools/setup_wsl.sh`, `tools/headless.sh`, `docs/re-notes.md`
- Modify: `.gitignore` (zusätzlich `build*/`, `out/`)

**Interfaces:**
- Produces: `~/cotl-deps/eden-duo/build/bin/eden-cli` mit `EDEN_DSMOD_BUILD_DEV_TOOLS=ON`; `~/cotl-deps/android-ndk-r28c`; `tools/headless.sh <save-dir> <btn-file> <out-prefix>` erzeugt `<out-prefix>-screen1.png` und `-screen2.png`.

- [ ] **Step 1:** `git init` im Projektordner, `.gitignore` ergänzen, Erst-Commit mit `.gitignore`, `docs/`.
- [ ] **Step 2:** Abhängigkeiten aus `~/cotl-deps/eden-duo` Build-Doku (Linux-Abschnitt) bei Tag `v1.3.2.2` ermitteln und als **eine** `sudo apt install …`-Zeile an den Nutzer geben; der Nutzer führt sie im Ubuntu-Terminal aus (Claude tippt kein Passwort).
- [ ] **Step 3:** `tools/setup_wsl.sh` schreiben: klont `igawa6/eden-duo` @ `v1.3.2.2` (mit Submodulen) und `igawa6/eden-duo-companions` nach `~/cotl-deps/`, lädt NDK r28c von `dl.google.com/android/repository/` nach `~/cotl-deps/`, konfiguriert Eden Duo mit `-DEDEN_DSMOD_BUILD_DEV_TOOLS=ON` und baut nur das Ziel `eden-cli` mit `-j4` (7 GB RAM in WSL).
- [ ] **Step 4:** Ausführen. Erwartet: `~/cotl-deps/eden-duo/build/bin/eden-cli --help` gibt Nutzung aus.
- [ ] **Step 5:** Keys und Spieldateien für eden-cli bereitstellen: `prod.keys`/`title.keys` in Edens Linux-Datenordner (`~/.local/share/eden/keys/`), Base-, Update- und DLC-NSPs installieren (`eden-cli`-Install-Option oder NAND-Import laut Eden-Doku). Spielstand aus `game/saves/01002E7016C46000/` in den Save-Ordner des Profils kopieren.
- [ ] **Step 6:** `tools/headless.sh` nach `PORTING_A_GAME.md` §7 schreiben (`EDEN_VSYNC=0`, `--aux-virtual`, `--screenshot-prefix`, `.btn`-Datei atomar, Screenshot über `<prefix>.shot`). Lauf, der bis in die Basis lädt.
- [ ] **Step 7:** Verifikation: `screen1.png` zeigt die Kult-Basis (Claude sieht das Bild an). Laufzeit bis Gameplay in `docs/re-notes.md` notieren.
- [ ] **Step 8:** Commit `chore: WSL toolchain and headless runner`.

### Task 2: Deklaratives Grundpaket mit Kult-Werten

**Files:**
- Create: `tools/gen_manifest.py`, `package/CultOfTheLamb/package.json`, `package/CultOfTheLamb/dualscreen/316A4D4881A9F1E0.json`, `tools/build.sh`
- Generated: `package/CultOfTheLamb/dualscreen/manifest.json`

**Interfaces:**
- Produces: `gen_manifest.py` mit Funktionen `tab_bar(active: str) -> list[dict]`, `page(id: str, widgets: list[dict], scrolls: list[dict] = []) -> dict`, `write_manifest(path: str) -> None`; Seiten-IDs `cult`, `problems`, `followers`, `inventory`, `dungeon`, `waiting`. `tools/build.sh [--no-module]` → `out/01002E7016C46000.dsmod.zip`.

- [ ] **Step 1:** Points in `316A4D4881A9F1E0.json` (Basis `["main+0x713AC50","0xB8","0xE0"]` = DataManager-Instanz): `dm` (`u64`, Instanz-Zeiger), `cult.faith` (`f32`, `0x354`), `cult.hunger` (`f32`, `0x340`), `cult.illness` (`f32`, `0x344`), `cult.illness_max` (`f32`, `0x348`), `g.day` (`s32`, `0x610`). Für jeden Wert ein `_v`-Kommentar mit Quelle.
- [ ] **Step 2:** Derived `g.ready` = `dm` ≠ 0. `page_binds`: `g.ready` == 0 → Seite `waiting` („Waiting for game…“).
- [ ] **Step 3:** `gen_manifest.py`: `format` 1, `title_id`, `canvas_w` 1240, `canvas_h` 1080, Tab-Leiste auf allen fünf Seiten (Buttons mit `page`-Aktionen), Seite `cult` mit drei `bar`-Widgets und Tag als `value`, übrige Seiten mit Platzhalter-Label.
- [ ] **Step 4:** `build.sh --no-module` ruft `gen_manifest.py` und `~/cotl-deps/eden-duo-companions/tools/build_dualscreen_package.py --package package/CultOfTheLamb --output out/ --version 0.1.0`.
- [ ] **Step 5:** Headless mit Paket in `~/.local/share/eden/dualscreen/…` (Installationspfad laut Eden-Doku). Verifikation: `screen2.png` zeigt Balken; Werte gegen Spiel-HUD auf `screen1.png`; Kontrollwert-Test: `writeb` auf `CultFaith` (Dev-Konsole) ändert die Anzeige, danach zurücksetzen, nicht speichern.
- [ ] **Step 6:** Nutzer installiert `out/…dsmod.zip` am Thor; Rückmeldung „Balken sichtbar, Werte stimmen“.
- [ ] **Step 7:** Commit `feat: declarative package with cult bars`.

### Task 3: Natives Modul-Grundgerüst und IL2CPP-Leser

**Files:**
- Create: `CMakeLists.txt`, `native/module.cpp`, `native/guest.h`, `native/guest.cpp`, `native/offsets_1_5_27.h`, `tests/test_util.h`, `tests/guest_test.cpp`
- Modify: `tools/build.sh` (Modul bauen + `--module` an den Paketbauer), `tools/gen_manifest.py` (`module`-Block, `requires_module`)

**Interfaces:**
- Produces (`guest.h`):
  - `struct Guest { virtual bool Read(uint64_t addr, void* out, size_t n) const = 0; };`
  - `class HostGuest : public Guest` (über `EdenDsmodHostApi`), `class FakeGuest : public Guest` (in `test_util.h`, `void Put(uint64_t addr, const void*, size_t)`).
  - `std::optional<uint64_t> ReadPtr(const Guest&, uint64_t addr);`
  - `template<class T> std::optional<T> ReadPod(const Guest&, uint64_t addr);`
  - `std::optional<std::vector<uint64_t>> ReadRefList(const Guest&, uint64_t list_obj, uint32_t max = 512);` (List<T> von Objekten)
  - `template<class T> std::optional<std::vector<T>> ReadValueList(const Guest&, uint64_t list_obj, uint32_t max = 512);` (List<int>, List<enum>)
  - `std::optional<std::string> ReadString(const Guest&, uint64_t str_obj, uint32_t max_chars = 256);` (UTF-16 → UTF-8)
  - `std::optional<uint64_t> StaticFields(const Guest&, uint64_t main_base, uint64_t typeinfo_off);`
- Produces (`module.cpp`): veröffentlicht `mod.ready` = 1 und `g.ready` überschreibt nicht.
- CMake-Ziel `dsmod-cotl` → `01002E7016C46000.so`, Exporte `eden_dsmod_get_module`, `eden_dsmod_get_extensions`; Testziel `cotl-tests` mit CTest.

- [ ] **Step 1: Failing tests** in `guest_test.cpp`: `ReadString` liefert `"Bramble"` für ein FakeGuest-String-Objekt (length 7 @0x10, UTF-16 @0x14); `ReadString` → `nullopt` bei length 300 > max; `ReadRefList` liefert 3 Zeiger aus `_items`/`_size`; `ReadRefList` → `nullopt` bei `_size` 513; `ReadRefList` → `nullopt` bei null `_items`; `ReadPtr` → `nullopt` auf ungemappter Adresse.
- [ ] **Step 2:** `cmake --build build-linux --target cotl-tests && ctest --test-dir build-linux` → FAIL (nicht definiert).
- [ ] **Step 3:** `guest.h/.cpp` implementieren. `CMakeLists.txt` nach dem Muster von `dsmod_add_module` in `eden-duo-companions/native/modules/CMakeLists.txt` (versteckte Sichtbarkeit, Version-Script, `--no-undefined`, Android `--exclude-libs,ALL`, Fehler ohne `c++_static`), `EDEN_SOURCE_ROOT` und `COMPANIONS_ROOT` als Cache-Variablen.
- [ ] **Step 4:** Tests → PASS.
- [ ] **Step 5:** `module.cpp`: `supports_build` nur für die exakte 64-stellige Build-ID; `create` prüft `HostAbiMatches` und alle 32 Bytes; `sample` fängt alles und veröffentlicht `mod.ready`. Test „Review Focus 1“: `snapshot`-freier Smoke-Test, dass `sample` mit FakeHost ohne gemappten Speicher `mod.ready`=1 und keine weiteren Werte veröffentlicht.
- [ ] **Step 6:** `build.sh` baut `linux-x86_64` und `android-arm64-v8a`, strippt mit `llvm-strip --strip-all`, packt mit `--module … --abi 1 --build-id <64 hex>`. Headless: Log zeigt Modul geladen, `mod.ready` sichtbar auf dem `__debug`-Panel (`debug_page: true` nur im Dev-Build).
- [ ] **Step 7:** Commit `feat: native module skeleton and IL2CPP readers`.

### Task 4: Follower-Snapshot und Follower-Tab

**Files:**
- Create: `native/snapshot.h`, `native/snapshot.cpp`, `native/publish.h`, `native/publish.cpp`, `tests/snapshot_test.cpp`, `tests/publish_test.cpp`
- Modify: `native/module.cpp`, `native/offsets_1_5_27.h`, `tools/gen_manifest.py` (Seite `followers`)

**Interfaces:**
- Produces (`snapshot.h`):
  ```cpp
  struct Follower { int32_t id; std::string name; int32_t level, age, life_expectancy;
    int32_t role, cursed_state, dwelling_id; bool old_age; std::vector<int32_t> traits;
    float satiation, starvation, faith, happiness, illness, injured, exhaustion, dissent;
    uint8_t group; /*0 normal,1 missionary,2 prisoner*/ };
  struct Objective { int32_t type, follower_id, item_type /*-1 wenn keine*/; float expire; bool complete, failed; };
  struct GameSnapshot { std::vector<Follower> followers; std::vector<Objective> objectives;
    std::vector<int32_t> offered_quest_follower_ids; /* Task 6 */
    std::vector<std::pair<int32_t,int32_t>> items; /* Task 7 */  struct Run* run = nullptr; /* Task 8 */ };
  std::optional<GameSnapshot> ReadSnapshot(const Guest&, uint64_t main_base);
  ```
  Offsets ausschließlich aus `offsets_1_5_27.h` (Werte aus `docs/recon-1.5.27.md`).
- Produces (`publish.h`): `struct Publisher { virtual void I64(const char*, int64_t)=0; virtual void F64(const char*, double)=0; virtual void Text(const char*, std::string_view)=0; };` und `void PublishFollowers(Publisher&, const GameSnapshot&);` (Werte laut Namensschema, Prozent 0–100 gerundet; Rolle und Traits vorerst als Zahl, Task 5 ergänzt den Parameter `const Terms&`).

- [ ] **Step 1: Failing tests** `snapshot_test.cpp`: FakeGuest mit DataManager, zwei Followern (Namen, Level, `_satiation` 0.8 …) → Felder korrekt; ein Follower-ID in `Followers_OnMissionary_IDs` → `group` 1; zweimaliges Lesen mit verändertem `_size` dazwischen (FakeGuest-Hook) → `nullopt` (Torn-Read); 60 Follower → 60 Einträge (Review Focus 2).
- [ ] **Step 2:** Tests → FAIL.
- [ ] **Step 3:** `ReadSnapshot` implementieren: Liste zweimal lesen, IDs vergleichen.
- [ ] **Step 4:** Tests → PASS.
- [ ] **Step 5: Failing tests** `publish_test.cpp`: `fol.count` 2; `fol.name0` „Bramble“; `fol.sat0` 80; normale Follower vor Gruppe 1/2 sortiert; Follower erscheint nicht zweimal, wenn er zugleich in Missionars- und Gefangenen-Liste steht (Review Focus 5, Gefangener gewinnt nach `re-notes`).
- [ ] **Step 6:** `PublishFollowers` implementieren → PASS.
- [ ] **Step 7: RE-Schritt:** Wertebereiche der Bedürfnisse aus dem Spielcode bestimmen (z. B. ob `_satiation` 0–100 oder 0–1, Max-Konstanten im Getter); Ergebnis + Quelle in `docs/re-notes.md`; Normalisierung in `PublishFollowers` danach anpassen und den Test-Erwartungswert mitziehen.
- [ ] **Step 8:** Seite `followers` in `gen_manifest.py`: Scroll-Region `fol.list` (`count_bind: fol.count`, `row_h` 96), Zeilen-Template mit `repeat`, vier `bar`s (Krankheit mit rotem Füllton), `on_tap` → `flag` `fol.open` = `{i}`; Detailzeile per `need_bind`/derived `fol.open == {i}`. Leerer Zustand: Label „No followers yet“ bei `fol.count` 0.
- [ ] **Step 9:** Headless mit Nutzer-Spielstand: Namen, Level und Balken gegen das Follower-Menü im Spiel (`.btn`-Skript öffnet das Menü) für mindestens drei Follower. Ergebnis in `re-notes.md`.
- [ ] **Step 10:** Commit `feat: follower list tab`.

### Task 5: I2-Texte und Enum-Begriffe

**Files:**
- Create: `tools/gen_enums.py`, `native/enums_1_5_27.inc`, `native/i2.h`, `native/i2.cpp`, `native/terms.h`, `native/terms.cpp`, `tests/i2_test.cpp`, `tests/terms_test.cpp`
- Modify: `CMakeLists.txt` (`chained_echoes_unity.cpp` aus `COMPANIONS_ROOT/native/modules`), `native/module.cpp`, `native/publish.cpp`

**Interfaces:**
- Produces:
  - `gen_enums.py <dump.cs> <out.inc>` erzeugt `COTL_ENUM(Name, Value, "Ident")`-Zeilen für `FollowerTrait.TraitType`, `InventoryItem.ITEM_TYPE`, `TarotCards.Card`, `EquipmentType`, `FollowerRole`, `Thought`.
  - `class I2Table { public: static std::optional<I2Table> Load(ce_unity::RangeReader resources_assets); std::optional<std::string> English(std::string_view term) const; size_t Size() const; };`
  - `class Terms { public: explicit Terms(const I2Table*); std::string Trait(int32_t) const; std::string Item(int32_t) const; std::string TarotName(int32_t) const; std::string TarotText(int32_t card, int32_t upgrade) const; std::string Weapon(int32_t) const; std::string Curse(int32_t) const; std::string Role(int32_t) const; std::string EnumName(std::string_view enum_name, int32_t) const; };` Fehlender Begriff → Enum-Ident (Review Focus 4), unbekannter Wert → `"#<wert>"`.

- [ ] **Step 1: RE-Schritt:** In `dump.cs`/Disassembly die Begriffsbildung finden (z. B. `FollowerTrait.GetLocalizedTitle`, `InventoryItem.LocalizedName`, `TarotCards.LocalisedName`, `EquipmentManager.GetEquipmentData(...).GetLocalisedTitle`): Präfix + Enum-Name + Suffix. Format pro Kategorie mit Quelle in `re-notes.md`.
- [ ] **Step 2:** `gen_enums.py` schreiben und ausführen; `.inc` committen.
- [ ] **Step 3: Failing tests** `i2_test.cpp` (übersprungen, wenn `COTL_ROMFS` nicht gesetzt): `Load` auf `work/romfs/Data/resources.assets` → `Size() > 1000`; `English("Inventory/Food")` nicht leer; unbekannter Begriff → `nullopt`. `terms_test.cpp` (ohne Romfs, mit Mini-`I2Table` aus Test-Konstruktor): `Trait(x)` liefert Text; fehlender Begriff → Enum-Ident; Wert 99999 → `"#99999"`.
- [ ] **Step 4:** Tests → FAIL.
- [ ] **Step 5:** `I2Table::Load` über `ce_unity::SerializedFile` (MonoBehaviour `I2Languages`; ohne TypeTree die Felder `mSource.mTerms[].Term`/`Languages[]` und `mLanguages[].Name` nach `LanguageSourceData`/`TermData` aus `dump.cs` roh parsen, Englisch-Spalte über `mLanguages[i].Name == "English"`). `Terms` nach Step 1.
- [ ] **Step 6:** Tests → PASS (mit `COTL_ROMFS=…/work/romfs`).
- [ ] **Step 7:** Signatur auf `PublishFollowers(Publisher&, const GameSnapshot&, const Terms&)` erweitern, `publish_test.cpp` anpassen. `module.cpp` lädt `I2Table` einmal lazy aus `romfs:Data/resources.assets` (Host-Reader), Follower-Tab zeigt Rollen und Traits in Details. Headless-Vergleich mit Spieltexten.
- [ ] **Step 8:** Commit `feat: English game texts via I2`.

### Task 6: Probleme und Quests

**Files:**
- Create: `native/problems.h`, `native/problems.cpp`, `tests/problems_test.cpp`
- Modify: `native/snapshot.cpp` (Objectives, angebotene Quests), `native/offsets_1_5_27.h`, `native/publish.cpp`, `native/module.cpp`, `tools/gen_manifest.py` (Seite `problems`, Zähler im Tab)

**Interfaces:**
- Consumes: `GameSnapshot`, `Terms`.
- Produces:
  ```cpp
  enum class ProblemKind : int32_t { Starving=0, Ill=1, Injured=2, Dissent=3, QuestAccepted=4,
    QuestOffered=5, Exhausted=6, Cursed=7, Old=8, Hungry=9 };
  struct Thresholds { float hungry, exhausted; /* aus re-notes, Quelle im Kommentar */ };
  struct ProblemRow { int32_t follower_id; ProblemKind kind; float urgency; std::string reason; };
  std::vector<ProblemRow> DeriveProblems(const GameSnapshot&, const Thresholds&, const Terms&, float now);
  void PublishProblems(Publisher&, const std::vector<ProblemRow>&, const GameSnapshot&);
  ```
  Ein Follower erscheint einmal, mit seinem dringendsten Problem; weitere Probleme im `reason`-Text („+2 more“).

- [ ] **Step 1: RE-Schritt:** Schwellen und Zustände aus dem Spiel ableiten: Hunger-/Erschöpfungswarnung (`FollowerBrain`-Getter bzw. `Thought`-Zustände wie `BecomeStarving`, `Ill`, `Dissenter`), Bedeutung von `CursedState`. Zeitbasis von `ObjectivesData.ExpireTimestamp` bestimmen (welche Spielzeit-Variable, z. B. in `TimeManager`) und als `now` an `DeriveProblems` übergeben. Angebotene Quests lokalisieren (Kandidaten: `FollowerBrain`/`FollowerInfo`-Felder mit Quest-Bezug, `Objectives`-Generator, Sprechblasen-Komponente); Verifikation per Kontrollwert und Spielstand mit sichtbarer Sprechblase. Ergebnis + Quelle in `re-notes.md`, `ProblemKind`-Tabelle oben aktualisieren.
- [ ] **Step 2:** Falls ein Spielstand fehlt (kranker Follower, Dissens, angebotene Quest, alter Follower): Nutzer gebündelt um Spielstände bitten.
- [ ] **Step 3: Failing tests** `problems_test.cpp`: kranker + hungernder + Dissens-Follower → Reihenfolge Starving, Ill, Dissent; angenommene Quest mit `follower_id` → `QuestAccepted`, Restzeit im `reason`; Quest ohne Follower → keine Zeile; angebotene Quest → `QuestOffered`; zwei Quests: kürzere Restzeit zuerst; Follower mit zwei Problemen → genau eine Zeile; 0 Follower → leere Liste; gesunder Follower → keine Zeile.
- [ ] **Step 4:** Tests → FAIL.
- [ ] **Step 5:** Implementieren → PASS.
- [ ] **Step 6:** Seite `problems`: Badges mit `prob.n_<kind>` (nur sichtbar > 0), Scroll-Liste wie im Mockup, Badge-Farbe je `prob.kind{i}`; leerer Zustand „Everyone is fine“; Tab-Label „Problems“ mit `prob.count` auf allen Seiten.
- [ ] **Step 7:** Headless mit den Spielständen aus Step 2: jedes angezeigte Problem gegen das Spiel geprüft, Ergebnis in `re-notes.md`.
- [ ] **Step 8:** Commit `feat: problems and quests tab`.

### Task 7: Inventar mit Icons und Quest-Marker

**Files:**
- Create: `native/icons.h`, `native/icons.cpp`
- Modify: `native/snapshot.cpp` (`DataManager.items`, `Objectives_CollectItem.ItemType` @ 0x78), `native/publish.cpp`, `native/module.cpp` (`load_image`), `tools/gen_manifest.py` (Seite `inventory`), `tests/publish_test.cpp`

**Interfaces:**
- Produces:
  - `void PublishInventory(Publisher&, const GameSnapshot&, const Terms&);` — Einträge mit `quantity > 0`, Reihenfolge wie im Spielinventar (RE-Schritt), `inv.quest{i}` = 1, wenn eine nicht abgeschlossene `Objectives_CollectItem` dieses `ItemType` verlangt (auch ohne Follower).
  - `class IconLoader { public: explicit IconLoader(ce_unity::RangeReader romfs); bool Load(std::string_view key, std::vector<uint8_t>& rgba, int& w, int& h); };` Schlüssel `module:cotl:item/<ident>`, `module:cotl:tarot/<ident>`, `module:cotl:weapon/<ident>`, `module:cotl:curse/<ident>`, `module:cotl:quest`. Unbekannt → neutrales generiertes Platzhalter-Icon (Review Focus 4).

- [ ] **Step 1: RE-Schritt:** Wo liegen Item-, Tarot-, Waffen- und Quest-Sprites (Sprite-Atlas in `resources.assets`/`sharedassets0.assets` oder Addressables-Bundle unter `StreamingAssets/aa/`) und wie bildet das Spiel Item → Sprite ab (`InventoryItem`-Icon-Getter / `InventoryIconMapping`). Quelle in `re-notes.md`.
- [ ] **Step 2: Failing tests** in `publish_test.cpp`: Items `[(LOG,12),(STONE,0),(BERRY,3)]` → `inv.count` 2; Sammel-Quest für BERRY → `inv.quest1`=1, `inv.quest0`=0; abgeschlossene Quest markiert nicht; leeres Inventar → `inv.count` 0.
- [ ] **Step 3:** → FAIL, implementieren, → PASS.
- [ ] **Step 4:** `IconLoader` mit `ce_unity::Reader::LoadSprite` (ASTC über Host-Decoder); Test (übersprungen ohne `COTL_ROMFS`): `Load("module:cotl:item/LOG")` liefert w,h > 0.
- [ ] **Step 5:** Seite `inventory`: Raster (`repeat_cols` 6), Icon, Menge, Quest-Symbol oben rechts bei `inv.quest{i}`; leerer Zustand „Inventory is empty“.
- [ ] **Step 6:** Headless: Mengen gegen Inventar im Spiel, Quest-Marker gegen Quest-Log. Leistung: Icon-Ladezeit im Log.
- [ ] **Step 7:** Commit `feat: inventory tab with icons and quest markers`.

### Task 8: Dungeon-Werte und automatischer Tab-Wechsel

**Files:**
- Modify: `native/snapshot.h/.cpp` (`struct Run`), `native/offsets_1_5_27.h`, `native/publish.cpp`, `package/CultOfTheLamb/dualscreen/316A4D4881A9F1E0.json`, `tools/gen_manifest.py` (Seite `dungeon`, `page_binds`), `tests/snapshot_test.cpp`, `tests/publish_test.cpp`

**Interfaces:**
- Produces:
  ```cpp
  struct Run { float hp_red, hp_red_max, hp_blue, hp_black, hp_spirit, hp_spirit_max, hp_fire, hp_ice;
    int32_t weapon, weapon_lvl, curse, curse_lvl, relic; std::vector<std::pair<int32_t,int32_t>> tarot; };
  ```
  `GameSnapshot::run` ist `std::optional<Run>` (ersetzt den Platzhalter-Zeiger), gelesen über `PlayerFarming.Instance` (Spieler 1; `players[0]`). `g.in_run` = 1 genau während eines Runs (Bedingung per RE-Schritt).
- Manifest: Flag `@flag:last_tab` (0–3), jede Tab-Aktion setzt es; `page_binds` auf `g.in_run`: `when_equal` → `dungeon`, `when_not_equal` → Seite `return`, die per `page_binds` auf `@flag:last_tab` sofort zur gemerkten Seite weiterleitet.

- [ ] **Step 1: RE-Schritt:** Zuverlässiges Run-Signal finden (z. B. `BiomeGenerator.Instance` ≠ 0 und `PlayerFarming.Location` = Dungeon; nicht in Basis/Woolhaven/Shops) und Relikt-Feld in `PlayerRelic`. Quelle in `re-notes.md`.
- [ ] **Step 2: Failing tests**: `ReadSnapshot` mit `PlayerFarming.Instance` = 0 → `run` leer, `g.in_run` 0; mit Spieler → Herzen korrekt; Koop-Fake mit zwei Spielern → nur `players[0]` (Review Focus 5); `PublishRun`: 4 Tarotkarten → `run.tarot_count` 4, Texte über `Terms`.
- [ ] **Step 3:** → FAIL, implementieren, → PASS.
- [ ] **Step 4:** Seite `dungeon`: Herzen-Reihe (`pips` je Herzart), Waffe/Fluch/Relikt mit Icon, Name, Level, Tarot-Raster mit `on_tap` → Detail-Overlay; Kartenfläche als Platzhalter für Task 9.
- [ ] **Step 5:** Headless-Szenario (Review Focus 3): Basis → Run starten → sterben → neuer Run → zurück zur Basis. Erwartet: Wechsel auf `dungeon` bei jedem Run-Start, nach Run-Ende genau einmal auf den zuvor gewählten Tab (z. B. `inventory`), Werte gegen HUD.
- [ ] **Step 6:** Commit `feat: dungeon tab and auto switch`.

### Task 9: Dungeon-Karte (nur Entdecktes)

**Files:**
- Create: `native/dungeon_map.h`, `native/dungeon_map.cpp`, `tests/dungeon_map_test.cpp`
- Modify: `native/snapshot.cpp` (`BiomeGenerator.Rooms`), `native/publish.cpp`, `tools/gen_manifest.py`

**Interfaces:**
- Produces:
  ```cpp
  struct Room { int32_t x, y; bool discovered, visited, boss, shop, respawn, special; bool n,e,s,w; };
  struct MapCell { int32_t px, py; int32_t state; int32_t kind; };
  struct MapLink { int32_t px, py, w, h; };
  struct MapLayout { std::vector<MapCell> cells; std::vector<MapLink> links; };
  MapLayout BuildMap(const std::vector<Room>&, int32_t cur_x, int32_t cur_y, int32_t area_w, int32_t area_h);
  ```
  Nur Räume mit `discovered || visited`; Verbindung nur, wenn beide Endräume sichtbar sind; Zellgröße so, dass das Raster in `area_w × area_h` passt (max. 96 px).

- [ ] **Step 1: Failing tests**: 3 Räume, einer nicht entdeckt → 2 Zellen, Verbindung zum versteckten Raum fehlt; aktueller Raum → `state` 3; Boss → `kind` 1 nur wenn entdeckt; negative Koordinaten werden auf die Fläche verschoben; leere Raumliste → leeres Layout.
- [ ] **Step 2:** → FAIL, implementieren, → PASS.
- [ ] **Step 3:** RE: Bedeutung von `Discovered` vs. `Visited` vs. `Hidden` am Spiel prüfen (Spielkarte vs. Companion, zwei Etagen inkl. Woolhaven).
- [ ] **Step 4:** Manifest: Zellen als `rect` mit `repeat` + `x_bind`/`y_bind`, Farbe nach `map.state{i}`, Icon nach `map.kind{i}`; Verbindungen als schmale `rect`s.
- [ ] **Step 5:** Headless: Karte nach jedem Raumwechsel mit Spielkarte (Pause-Menü) vergleichen; kein unentdeckter Raum sichtbar.
- [ ] **Step 6:** Commit `feat: discovered-only dungeon map`.

### Task 10: Kult-Tab vervollständigen

**Files:**
- Modify: `native/publish.cpp`, `native/snapshot.cpp`, `package/CultOfTheLamb/dualscreen/316A4D4881A9F1E0.json`, `tools/gen_manifest.py`, `tests/publish_test.cpp`

**Interfaces:**
- Produces: `cult.alive`, `cult.old`, `cult.sick`, `g.cult_level` (Text über `Terms::EnumName("CultLevel", …)`), Kultname, Tageszeit (RE: `TimeManager`), `cult.storage_pct`; optional `cult.dirt`.

- [ ] **Step 1: Failing tests**: Zähler aus Snapshot (2 lebend, 1 alt, 1 krank).
- [ ] **Step 2:** → FAIL, implementieren, → PASS.
- [ ] **Step 3:** RE Tageszeit und Lager-Füllstand (`StructuresData`/Lagergebäude); Schmutz nur, wenn Aufwand ≤ halber Tag, sonst als offen in `re-notes.md` vermerken.
- [ ] **Step 4:** Manifest-Seite `cult` final; Headless-Vergleich.
- [ ] **Step 5:** Commit `feat: complete cult tab`.

### Task 11: Abnahme auf dem Thor

**Files:**
- Modify: `tools/gen_manifest.py` (`debug_page` aus), `package/CultOfTheLamb/package.json` (Version 1.0.0), `docs/re-notes.md`

- [ ] **Step 1:** `build.sh` Release; Nutzer installiert am Thor.
- [ ] **Step 2:** Nutzer spielt 30 min (Basis + Run). Mit scrcpy: alle fünf Tabs, Auto-Wechsel, Karte, Icons.
- [ ] **Step 3:** Leistung: `@perf.fps` mit und ohne zweiten Bildschirm vergleichen; Ziel: kein messbarer Einbruch (< 2 fps Differenz im Mittel). Sonst Profiling (`EDEN_DSMOD_PROFILE_WIDGETS=1` am PC) und Abtastrate des Moduls senken (Listen nur alle 15 Ticks lesen).
- [ ] **Step 4:** Liste aller Abweichungen vom Spiel in `re-notes.md`; jede beheben oder mit Nutzer klären.
- [ ] **Step 5:** Commit `release: v1.0.0`, Tag `v1.0.0`.
