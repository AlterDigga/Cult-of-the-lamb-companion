# Cult of the Lamb – Dual-Screen-Companion für Eden Duo

Stand: 2026-10-31 · Ziel-Spielversion 1.5.27 · Zielgerät AYN Thor (Android, zweiter Bildschirm)

## 1. Absicht

Der Spieler nutzt Cult of the Lamb auf dem AYN Thor im Emulator Eden Duo. Der untere Bildschirm
soll ohne Pausieren und ohne Menüs zeigen, was im Kult los ist und wie der laufende Dungeon-Run
steht. Kernnutzen: auf einen Blick sehen, **welcher Follower gerade ein Problem hat oder eine Quest
offen hat**, und im Dungeon Herzen, Ausrüstung, Tarotkarten und die bisher entdeckte Karte.

Das Ergebnis ist ein Companion-Paket `01002E7016C46000.dsmod.zip` im Format von
`igawa6/eden-duo-companions`. Es wird am Thor über *Add-ons → Install → Dual screen mods*
installiert.

## 2. Ziele, Nicht-Ziele, Anti-Ziele

**Ziele**
- Fünf Tabs: Kult, Probleme, Follower, Inventar, Dungeon (Inhalte in §5)
- Automatischer Wechsel auf den Dungeon-Tab bei Run-Start und zurück auf den vorherigen Tab danach
- Texte auf **Englisch** (Nutzer spielt auf Englisch); Icons aus den Spieldateien des Spielers
- Flüssig auf dem Thor: das Spiel selbst darf messbar nicht langsamer werden

**Nicht-Ziele (vorerst)**
- Touch-Aktionen, die das Spiel steuern (Tastendrücke, Speicher-Schreibzugriffe). Vom Nutzer
  ausdrücklich zurückgestellt.
- Andere Spielversionen als 1.5.27
- Koop: Es wird nur Spieler 1 angezeigt.
- Andere Sprachen als Englisch (Entscheidung des Nutzers). Der Code soll die Sprache nicht so fest
  verdrahten, dass eine spätere Erweiterung einen Umbau braucht, aber es wird nichts dafür gebaut
  oder getestet.
- Veröffentlichung oder Beitrag zu `igawa6/eden-duo-companions` (ursprünglicher Stand, später vom Nutzer aufgehoben): rein privater Gebrauch. Ein
  eigenständiger Aufbau in diesem Ordner genügt; er darf Werkzeuge und Helfer aus dem Repo nutzen.

**Anti-Ziele (wäre ein Fehlschlag, selbst wenn es technisch läuft)**
- Spoiler oder Cheat-Informationen: Die Karte zeigt **nur Entdecktes**.
- Falsche Werte. Jeder angezeigte Wert muss mit der Anzeige im Spiel übereinstimmen.
- Das Spiel verändern. Der Companion ist reiner Lesezugriff.
- Spielinhalte mitliefern: keine Texte, Bilder oder Daten des Spiels im Paket.
- Nur auf dem PC funktionieren. Maßgeblich ist das Verhalten auf dem Thor.

## 3. Rahmenbedingungen

- **Laufzeit:** Auf dem Thor läuft **Eden Duo 1.3.2.2** (Angabe des Nutzers), also mindestens
  Runtime 22 / Modul-ABI 1. Alle Runtime-22-Funktionen (`preload`, zweiachsiges Scrollen,
  `"$tap"`, neue Karten-Keys) stehen zur Verfügung. Das Paket deklariert das niedrigste
  `min_runtime`, das die genutzten Funktionen abdeckt. Die Entwicklungs-Version von Eden Duo in
  WSL soll derselben Version entsprechen.
- **Android nutzt NCE:** `call`/`sequence` (Funktionsaufrufe im Spiel) und `spies` stehen dort
  nicht zur Verfügung. Alles muss über Speicher-Lesen gehen.
- **Version gebunden** an Build-ID `316A4D4881A9F1E0…` (Datei `316A4D4881A9F1E0.json`).
  Bei anderer Build-ID lädt der Companion nicht und zeigt einen Hinweis.
- **Canvas** 1240 × 1080 wie bei den veröffentlichten Paketen.
- **Unity 2022.3.62f3, IL2CPP v31.** Offsets und Singletons stehen in `docs/recon-1.5.27.md`.
  Die Lage von `static_fields` in `Il2CppClass` muss live bestätigt werden.
- **DLC:** Alle DLCs des Nutzers sind installiert, inkl. Woolhaven (Dungeons 5/6). Karte und
  Inventar müssen damit umgehen.
- **Entwicklungsumgebung:** Windows 11 + WSL2 Ubuntu 24.04 (sudo vorhanden, der Nutzer gibt das
  Passwort selbst ein). Spieldateien, Keys und Spielstand liegen unter `game/`, Dumps unter
  `work/`, beides nicht versioniert.
- **Rechtliches:** Nur die eigene Kopie des Nutzers. Das Paket enthält keine Spielinhalte und
  liest alles zur Laufzeit aus `romfs:`/`aoc:`.

## 4. Bauweise

**Betrachtete Ansätze**

| | Ansatz | Abwägung |
|---|---|---|
| A | Nur Manifest + Adressketten, kein Code | Schnell, aber keine gefilterte/sortierte Probleme-Liste, keine Unity-Icons, keine lokalisierten Texte |
| **B** | **Manifest-Layout + natives C++-Modul für Daten, Texte und Bilder** | Erfüllt alle Anforderungen; Vorlage: Dave the Diver (`native/modules/dtd`). Mehr Aufwand (arm64-Build) |
| C | Modul zeichnet ganze Tabs als Bilder | Volle Freiheit, aber viel mehr Code und Last; eingebaute Scroll-/Tap-Logik ginge verloren |

**Gewählt: B** (Entscheidung des Nutzers, auf Empfehlung).

**Aufteilung bei B**
- **Per-build-Datei (`points`):** einfache Skalare direkt über Adressketten: Kult-Balken, Tag,
  Herzen, Waffe/Fluch + Level, Listengrößen, Run-Status für den Tab-Wechsel.
- **Natives Modul** (`modules/android-arm64-v8a/01002E7016C46000.so`, zusätzlich
  `linux-x86_64` für Tests am PC):
  - Follower-Liste lesen, Probleme ableiten, filtern, sortieren; Quests (`DataManager.Objectives`)
    den Followern zuordnen
  - Inventar lesen; Quest-Gegenstände aus `Objectives_CollectItem.ItemType` markieren
  - I2-Localization (`I2Languages` in `romfs:/Data/resources.assets`) lesen, englische Spalte,
    für Namen von Eigenschaften, Rollen, Gegenständen, Tarotkarten, Waffen, Flüchen
  - Unity-Sprites (Item-, Tarot-, Quest-Icons) aus den Unity-Dateien dekodieren und über
    `load_image` als `module:`-Bilder liefern
  - Raumliste des `BiomeGenerator` in Karten-Daten umsetzen, nur entdeckte Räume
  - Ergebnisse als Modul-Ausgaben (Zahlen/Texte pro Zeile) veröffentlichen; das Manifest zeigt
    sie über `repeat` + `scroll`
- **Manifest:** Seiten, Widgets, Scroll-Regionen, Tab-Leiste, `page_binds` für den Dungeon-Wechsel.

## 5. Inhalte (vom Nutzer entschieden, Vorschläge markiert)

**Tab-Leiste** oben auf jeder Seite: Kult · Probleme *n* · Follower *n* · Inventar · Dungeon.
Die Zahl bei „Probleme“ ist auch im Dungeon sichtbar *(Vorschlag, angenommen)*.

### 5.1 Kult *(Inhalt: Vorschlag von Claude, vom Nutzer angenommen)*
- Große Balken: Glaube (`CultFaith`), Hunger (`HungerBarCount`), Krankheit
  (`IllnessBarCount` / `IllnessBarDynamicMax`)
- Tag (`CurrentDayIndex`) und Tageszeit, Kult-Level, Kultname
- Follower lebend / alt / krank, Füllstand der Lager
- Schmutz (Kot/Erbrochenes) aus Gebäudedaten: **optional, zuletzt**

### 5.2 Probleme
- **Nutzer:** nur Follower mit Problem oder offener Quest.
- Zähler-Badges pro Problemart oben, darunter je Follower eine Zeile: Avatar-Kürzel, Name, Level,
  kurzer Grund, Badge.
- *Vorschlag, angenommen:* Problemarten: krank, Dissens, hungrig, verletzt, erschöpft, alt,
  verflucht, offene Quest.
- *Vorschlag, angenommen:* Reihenfolge: lebensbedrohlich (Krankheit, Hunger) → Dissens →
  Quests nach Restzeit → Alter.
- *Vorschlag, angenommen:* Schwellen so wie die Warnungen im Spiel. Der Builder liest sie aus
  der Spiellogik ab, statt sie zu schätzen.
- **Nutzer:** Als „offene Quest“ zählen **angenommene** Quests (`DataManager.Objectives` mit
  zugeordnetem Follower) **und angebotene** Quests (Follower mit Quest-Sprechblase, noch nicht
  angenommen). Wo das Angebot im Speicher liegt, muss der Builder noch finden; es steht nicht in
  `recon-1.5.27.md`.
- Quest-Zeile zeigt Quest-Text bzw. -Typ und, bei angenommenen Quests, die Restzeit
  (`ExpireTimestamp`). Angebotene Quests tragen einen eigenen Badge („angeboten“).

### 5.3 Follower
- **Nutzer:** alle Follower, scrollbar.
- Pro Zeile Name, Level, Rolle und **vier Balken: Sattheit, Glaube, Krankheit, Glück** (Nutzer).
- *Vorschlag, angenommen:* Krankheit mit umgekehrter Farbskala (voll = rot).
- *Vorschlag, angenommen:* Antippen klappt Details auf: Alter / Lebenserwartung, Eigenschaften,
  Unterkunft, Partner.
- *Vorschlag, angenommen:* Tote nicht gelistet; Missionare und Gefangene als eigene Gruppe am Ende.

### 5.4 Inventar
- **Nutzer:** einfache Liste aller vorhandenen Gegenstände mit Menge.
- **Nutzer:** Gegenstände, die eine offene Quest braucht, tragen oben rechts das Quest-Symbol
  wie im Spiel.
- *Entscheidung Claude:* Maßgeblich sind alle **angenommenen** Sammel-Quests, auch Story- und
  NPC-Quests ohne Follower. So verhält sich nach Verständnis das Spiel selbst; weicht das Spiel
  ab, gilt das Spiel. Angebotene, noch nicht angenommene Quests markieren keine Gegenstände.
- Icons und Namen aus den Spieldateien.

### 5.5 Dungeon
- **Nutzer:** Wechsel automatisch bei Run-Start und zurück danach.
- *Entscheidung Claude:* `page_binds` springen nur auf feste Seiten. Der zuletzt offene Tab wird
  deshalb in einem Laufzeit-Flag gemerkt; beim Run-Ende wird dorthin zurückgesprungen.
- Herzen-Reihe: rot, blau, schwarz, Geist, Feuer, Eis (`HealthPlayer`)
- Waffe und Fluch mit Name, Icon, Level; Relikt, falls vorhanden
- Tarotkarten als Raster mit Name und Stufe; Antippen zeigt die Wirkung.
- **Nutzer:** Karte zeigt **nur entdeckte Räume**. *Vorschlag, angenommen:* gesehene, nicht
  betretene Räume blasser; aktueller Raum hervorgehoben; Symbole für Boss, Shop, Sonderräume.

### 5.6 Gestaltung
Ein Mockup der Tabs Probleme und Follower wurde gezeigt und nicht beanstandet. Optik im Stil des
Spiels (dunkler Hintergrund, Spiel-Schrift wo möglich) ist Sache des Builders.

## 6. Prüfung

- Jeder Wert wird gegen die Anzeige im Spiel geprüft (Screenshot Spiel vs. Companion), mit
  Kontrollwert-Test laut `PORTING_A_GAME.md` und an zwei Spielständen bzw. zwei Starts.
- **Spielsituationen:** Der Nutzer legt auf Anfrage Spielstände in `game/saves/` ab (Zusage:
  „ein paar“). Der Builder fragt gezielt und gebündelt nach den benötigten Situationen (z. B.
  kranker Follower, angebotene Quest, laufender Run in Woolhaven) und nutzt pro Spielstand so
  viele Prüfungen wie möglich. Was sich ohne Spielstand per Eingabe-Skript (`.btn`) erreichen
  lässt, stellt der Builder selbst her.
- Entwicklung headless mit `eden-cli --aux-virtual` in WSL; Abschluss am Thor (scrcpy).
- Leistung am Thor mit `EDEN_DSMOD_PROFILE` bzw. den Profiling-Flags prüfen.

## 7. Dem Builder überlassen

- Genaue Adressketten, Modul-Struktur, Wahl zwischen `points` und Modul pro Wert
- Layout-Maße, Farben, Animationen, Schriften
- Wie das Modul Unity-Assets und I2-Daten parst (eigener Parser oder Bibliothek, sofern die
  Lizenz zu GPL-3.0 passt)
- Reihenfolge der Umsetzung; Empfehlung: Kult → Follower → Probleme → Inventar → Dungeon-Werte →
  Karte → Schmutz

## 8. Änderungen nach Thor-Test 2 (Paket 0.5.0)

Vom Nutzer nach dem Test gewünscht:

- **Eigener Quests-Tab** (6 Tabs: Cult, Problems, Quests, Followers, Inventory, Dungeon). Zeigt
  **alle** angenommenen Quests, auch Story- und NPC-Quests ohne Follower, dazu oben die
  angebotenen. Antippen öffnet ein Fenster mit Questtext und Fortschritt.
  *Entscheidung Claude:* Quests (angenommen und angeboten) erscheinen nicht mehr im Problems-Tab.
- **Problems-Tab:** Antippen eines Zähler-Badges filtert die Liste auf diese Art; erneutes
  Antippen hebt den Filter auf.
- **Follower-Details:** zusätzlich das Level; ein kleines Feld öffnet ein schwebendes Fenster mit
  den „Thoughts“ (Gedanken) des Followers.
- **Tagesbalken** in die fünf Tagesphasen eingefärbt (Morgen gelb, Mittag orange, Abend dunkelblau
  usw.) mit Markierung der aktuellen Zeit.
- **Größere Schrift überall**, höhere Zeilen; Scrollen, wo nicht alles passt.
