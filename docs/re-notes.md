# RE-Notizen Cult of the Lamb 1.5.27

Jeder Fund mit Quelle und Verifikation. Offsets siehe `recon-1.5.27.md`.

## Entwicklungsumgebung (Task 1)

- WSL-Distro **Ubuntu 26.04** (Eden Duo v1.3.2.2 verlangt ≥ 26.04), `eden-cli` aus
  `~/cotl-deps/eden-duo/build/bin/`, gebaut mit `ENABLE_QT=OFF`, `ENABLE_WERROR=OFF` (GCC 15).
- **Kein GPU-Rendering in WSL:** Ubuntus Mesa hat keinen d3d12/Dozen-Vulkan-Treiber.
  Mit lavapipe stürzt `eden-cli` in `libvulkan_lvp.so` ab (gdb-Backtrace, Thread „eden-cli“,
  nach ~74 normal behandelten Fastmem-SIGSEGVs der CPU-Kerne). OpenGL über llvmpipe
  (`MESA_GL_VERSION_OVERRIDE=4.6COMPAT`) bleibt beim Shader-Kompilieren nach ~18 s hängen.
- **Lösung:** `--null-render`. Spiel läuft mit 60 fps, `screen2.png` (Companion) wird erzeugt,
  `screen1.png` (Spielbild) bleibt leer. Abgleich mit der Spielanzeige deshalb über
  Speicherwerte, Spielstand und am Thor.
- Getrenntes Eden-Profil: `XDG_DATA_HOME=~/cotl-run/data`, Konfiguration `~/cotl-run/sdl-config.ini`
  (Fastmem aus, siehe `setconf.py`-Aufrufe in der Session; eden-cli schreibt die Datei beim Start neu).
- Save-Pfad: `nand/user/save/0000000000000000/<uuid>/01002E7016C46000`, `<uuid>` =
  `{u64@0x18:016X}{u64@0x10:016X}` aus `nand/system/save/8000000000000010/su/avators/profiles.dat`
  (Format aus `SaveDataFactory::GetFullPath`). In `tools/headless.sh` eingebaut.
- Timing (Null-Renderer): Titelbild nach ~35 s; `tools/btn/load_save.btn` (5 × A im Abstand 4 s)
  lädt den Spielstand, ~60 s nach Start existiert die neue `DataManager`-Instanz.

## Singletons

- `DataManager.instance`: `chain 713AC50 0xB8 0xE0` → Instanz. Verifiziert 2026-10-31:
  null vor dem Titel, Default-Instanz am Titel, neue Instanz nach dem Laden. Damit ist
  `Il2CppClass.static_fields` @ **0xB8** bestätigt.

## Kult-Werte (Task 2)

Quelle: Getter disassembliert mit `tools/re/dis.sh` (capstone + lz4 per `apt-get download` nach
`~/cotl-deps/py`, ohne sudo).

| Wert | Getter | Feld | Bereich |
|---|---|---|---|
| Glaube | `CultFaithManager.get_CurrentFaith` @0x2100150 | `DataManager+0x354` | 0–85 (`MAX_FAITH`), Start 50, niedrig ≤ 5 |
| Hunger | `HungerBar.get_Count` @0x2B16290 | `+0x340` | 0 (`get_MIN_HUNGER`) – 100 (`get_MAX_HUNGER` = 0x42C80000) |
| Krankheit | `IllnessBar.get_Count` @0x2B336F0 | `+0x344` | 0 – `get_DynamicMax` (`+0x348`) |

Verifikation 2026-10-31 (Spielstand des Nutzers, headless): Tag 9, Glaube 10,95, Hunger 64,8,
Krankheit 0/10. Kontrollwert: `writeb` auf die oberen Bytes von `CultFaith` → 50,04, Companion
zeigt 58 % (= 50/85). Beim Tageswechsel 9 → 10 fiel der Glaube im Lauf auf 0.
Abgleich mit der Spielanzeige steht aus (am Thor).

## Follower (Task 4)

- Bedürfnisse: `FollowerInfo.set_Satiation/Faith/Illness/Happiness` begrenzen per `fcsel` auf
  [0, 100] (Konstante 0x42C80000) → Werte direkt als Prozent.
- Level = `FollowerInfo.XPLevel` (+0x12C). Abgleich mit der Anzeige im Spiel steht aus (Thor).
- Verifikation headless (Spielstand des Nutzers): 7 Follower – Mergrebre (Lv 2, Alter 33/50),
  Hanojul, Gremer (Sattheit 0), Thorgre (Lv 2), Valefar, Armer, Gusion.
- Manifest: Listen-Templates brauchen `repeat` (Maximum) zusätzlich zu `repeat_bind`; ein Tipp
  liefert `$payload` nur mit `select_group` am Widget (so auch P5R/ACNH).

## Thor-Rückmeldung 1 (Paket 0.3.0)

- Modul lädt unter NCE, Werte kommen an, Spiel bleibt flüssig. Tag, Hunger, Follower-Level und
  -Alter stimmen mit dem Spiel überein (Nutzer).
- Krankheit wird im Spiel umgekehrt dargestellt: `IllnessBar.get_IllnessNormalized` =
  `1 − Count/Max` (voll = gesund) → Companion zeigt jetzt „Health“ wie das Spiel.
- Glaube: Die HUD-Leiste (`CultFaithManager.UpdateBar`) nutzt `get_CultFaithNormalised` =
  `CultFaith / 85` – identisch zum Companion.
- Tageszeit: `DataManager+0x614` Phase (`TimeManager.get_CurrentPhase`, DayPhase 0–4),
  `+0x618` Spielzeit (`get_CurrentGameTime`, 0–1200 Spielminuten).
- Heller Rand auf dem Thor: `DrawRect` zeichnet `max(1, frame)` px Rahmen, solange `color` ≠ 0
  → alle Rechtecke mit `color` `#00000000`.
- Lebenserwartung wird nicht angezeigt (Wunsch des Nutzers).
- Loyalität statt Follower-Glaube (Wunsch des Nutzers): `FollowerInfo.Adoration` (+0x128),
  gelesen von `FollowerBrainStats.get_Adoration`, Maximum 100 (`get_MAX_ADORATION`).
  Headless: Mergrebre 12 (Lv 2), Hanojul 80, Gremer 65, Thorgre 0 (Lv 2), Valefar 30, Armer 20,
  Gusion 20 – Level-2-Follower niedrig, da die Leiste nach dem Aufstieg neu beginnt.

## I2-Begriffe (Task 5)

Aus den String-Literalen der Getter (`tools/re/dis.sh` löst StringLiteral-Slots auf):

| Kategorie | Getter | Begriff |
|---|---|---|
| Eigenschaft | `FollowerTrait.GetLocalizedTitle` @0x2A9A560 | `Traits/{TraitType}` |
| Rolle | `FollowerRoleInfo.GetLocalizedName` @0x29C83A0 | `Traits/{FollowerRole}` |
| Gegenstand | `InventoryItem.LocalizedName` @0x1BC47A0 | `Inventory/{ITEM_TYPE}` |
| Tarot-Name | `TarotCards.LocalisedName` @0x1F6C1D0 | `TarotCards/{Card}/Name` |
| Tarot-Text | `TarotCards.LocalisedDescription` @0x1F6C580 | `TarotCards/{Card}/Description` |
| Waffe/Fluch | `EquipmentData.GetLocalisedTitle` @0x2935180 | `UpgradeSystem/{EquipmentType}/Name` |
| Relikt | `RelicData.GetTitleLocalisation` @0x1D970F0 | `Relics/{RelicType}` (+ `/Cleansed`) |

### I2Languages-Layout (gemessen, ohne TypeTree)

MonoBehaviour `I2Languages` in `Data/resources.assets` (12,4 MB, Klasse 114). Nach dem
MonoBehaviour-Kopf (28 Byte) und `m_Name`: 3 bools (je auf 4 Byte ausgerichtet), `mTerms`
(11245 Einträge: `Term`, `TermType`, `Languages[]` 21×, `Flags` byte[] + Ausrichtung,
`Languages_Touch[]`), `CaseInsensitiveTerms`, `OnMissingTranslation`, `mTerm_AppName`,
`mLanguages` (21: `Name`, `Code`, `Flags`-Byte ausgerichtet). **Nicht gespeichert** (trotz
dump.cs): `mIsGlobalSource`, `TermData.Description`, `LanguageData.Compressed` – sie sind
`[NonSerialized]`, was Il2CppDumper nicht anzeigt. Englisch = Spalte 3 (davor Description,
Comments, Max Char Limit). Beispiele: `Inventory/BERRY` = Berry, `Traits/Worker` = Worker,
`TarotCards/Hearts1/Name` = The Hearts I, `UpgradeSystem/Sword/Name` liefert den Waffennamen.
Headless: Name, Rolle, Alter und Eigenschaften stimmen mit dem Spiel überein.

## Probleme und Quests (Task 6)

- Zustand: `FollowerInfo.CursedState` (Thought): Dissenter 66, Ill 68, BecomeStarving 131,
  OldAge 146, Zombie 181, Injured 310, Freezing 480, Overheating 481, Soaking 482, Aflame 483.
- Schwellen (`FollowerInfo`-Konstanten): `HUNGER_THRESHOLD` 30 (hungrig darunter),
  `MAX_STARVATION` 75, `DISSENTER_THRESHOLD` 80. Erschöpft: `FollowerBrainStats.set_Exhaustion`
  wechselt den Zustand beim Übergang über 0 → erschöpft = `Exhaustion > 0`.
- Angenommene Quests: `DataManager.Objectives` (+0xE38), `ObjectivesData`: Type 0x10,
  IsComplete 0x20, IsFailed 0x21, QuestExpireDuration 0x2C, ExpireTimestamp 0x30, Follower 0x48;
  `Objectives_CollectItem.ItemType` 0x78 (Type 1 = COLLECT_ITEM).
- Restzeit: `ObjectivesData.get_ExpiryTimeNormalized` = (ExpireTimestamp − TotalElapsedGameTime) /
  QuestExpireDuration; `TotalElapsedGameTime` = CurrentDayIndex × 1200 + CurrentGameTime.
- Angebotene Quests: `FollowerBrain.AllBrains` (statisch +0x40, TypeInfo main+0x713C068), je Brain
  `_currentTask` (+0xC8) vom Typ `FollowerTask_GetAttention` (Klasse über TypeInfo main+0x713C3C0)
  mit `ComplaintType` (+0x68) = 8 (GiveQuest); Follower über `_directInfoAccess` (+0x20).
  FollowerBrain wird spät initialisiert (TypeInfo-Slot zunächst Metadaten-Token) → dann leer.
- Headless (Spielstand des Nutzers): Gremer „Starving“, Thorgre „Exhausted, +1 more“ (Quest);
  5 Objectives, 3 davon Story/NPC ohne Follower. Angebotene Quests, Krankheit, Dissens, Alter:
  im Spielstand nicht vorhanden → Prüfung mit weiteren Spielständen bzw. am Thor ausstehend.
- Darstellung: Die eingebaute Schrift hat keine Klammern; zentrierte Labels nehmen x als Mitte.

## Quests und Gedanken (Thor-Wunsch 2)

- Quest-Texte: Jede `Objectives_*`-Unterklasse baut ihren Text in `get_Text` (36 Klassen).
  Nachgebaut: COLLECT_ITEM `Objectives/CollectItem` (ItemType 0x78, Target 0x80, Count 0x88),
  BUILD_STRUCTURE `Objectives/BuildStructure[/Plural]` (StructureType 0x78, Target 0x7C,
  Count 0x80), RECRUIT_FOLLOWER `Objectives/RecruitFollower/Plural` (Count 0x78), CUSTOM
  `Objectives/Custom/{CustomQuestTypes}` (0x78, Zielfollower 0x7C), SEND_FOLLOWER_BED_REST
  `Objectives/SendFollowerToBedRest` (string 0x78), COOK_MEALS `Objectives/CookMeals`
  (MealType 0x78, Count 0x7C), UNLOCK `Objectives/Unlock/{UpgradeSystem.Type}` (0x78).
  Übrige Typen: lesbarer Typname. Titel: I2-Begriff in `ObjectivesData.GroupId` (+0x18).
- Namen in Platzhaltern: `Structures/{TYPES}` (StructuresData.GetLocalizedNameStatic),
  `CookingData/{ITEM_TYPE}/Name` (CookingData.GetLocalizedName).
- Gedanken: `FollowerInfo.Thoughts` (+0x120) → `ThoughtData` (ThoughtType 0x10, Modifier 0x18,
  Quantity 0x24); Text `FollowerThoughts/{Thought}`. Spieltexte enthalten `<sprite …>`-Markup →
  wird für alle Begriffe entfernt.
- Headless (Spielstand des Nutzers): 5 Quests, Titel und Ziele stimmen mit dem Spiel überein.
- Gruppierung wie `Lamb.UI.QuestsMenu.OnShowStarted`: Dictionary über `UniqueGroupID`
  (`ObjectivesData` +0x40, string) aus `DataManager.Objectives` (+0xE38) und
  `DataManager.CompletedObjectives` (+0xE40). Ein Quest = alle Objectives einer Gruppe; erledigte
  Unterpunkte stammen aus der zweiten Liste. Gruppen ohne offenen Unterpunkt werden nicht gezeigt.
- PERFORM_RITUAL: `Objectives/PerformRitual/{UpgradeSystem.Type}`, {0}/{1} = Namen der Follower
  in +0x7C/+0x80 (`Objectives_PerformRitual.get_Text` @0x1C7C6xx). Blutmond = `Ritual_Halloween`.
- Headless: 4 Quests statt 5 Einträgen; eine Quest mit zwei Zielen,
  eine mit einem offenen und drei erledigten Zielen.

## Inventar und Icons (Task 7)

- Inventar: `DataManager.items` (+0xE80) → `List<InventoryItem>` (type 0x10, quantity 0x14).
  Headless: 16 Gegenstände, u. a. Coin ×237, Bone ×96, Stone ×2.
- Icon-Zuordnung: MonoBehaviour „Inventory Icon Mapping“ (`InventoryIconMapping`, path_id 27700
  in `resources.assets`): `_itemImages` = Liste (ItemType int, Sprite PPtr), 203 Einträge.
- Sprite (Klasse 213) ohne TypeTree, Rohlayout Unity 2022.3: m_Name, m_Rect, m_Offset, m_Border,
  m_PixelsToUnits, m_Pivot, m_Extrude, m_IsPolygon (ausgerichtet), m_RenderDataKey (GUID 4×u32 +
  int64), m_AtlasTags. Die Inventar-Sprites sind in spät gebundene SpriteAtlanten gepackt
  (eigener Textur-Verweis = 0).
- Atlanten liegen in Addressables-Bundles (mit TypeTree): `tools/gen_atlas_bundles.sh` scannt alle
  ~10.300 Bundles (≈2 min) → `native/atlas_bundles_1_5_27.inc` (627 Atlanten). Resources_Atlas =
  `f70416a55415bfa877b16a7e48f9c324.bundle`. `m_RenderDataMap`-Eintrag:
  `[[GUID{data[0..3]}, second], SpriteAtlasData{texture, textureRect, settingsRaw, …}]`.
- Abdeckung: 190 von 203 Icons über Atlanten. 11 weitere Sprites haben eine eigene Textur in
  resources.assets (ohne TypeTree, Daten in `resources.assets.resS`): `m_RD.texture` steht hinter
  m_AtlasTags und m_SpriteAtlas, Ausschnitt = m_Rect. Alle ASTC 12×12 im Tegra-Blocklayout;
  `m_PlatformBlob[8]` = log2(GOBs je Block). Auch bei 0 (1 GOB) ist die Textur geswizzelt, das hält
  `ce_unity::DecodeTextureData` für linear; IconTexture (4096²) übersteigt dessen Speichergrenze.
  Daher `DecodeSwitchSprite`, die nur die Blöcke des Ausschnitts dekodiert. Rest: Soul_Icon (Tag
  HUD, zwei Einträge) ohne Icon.
- ASTC: Atlas 2048×2048 ASTC 12×12 ist auf 2052×2052 gepolstert. `ce_unity::MakeHostAstcDecoder`
  lehnt > 16 MiB ab, Edens `decode_astc` Breite/Höhe > 2048 → eigener Kachel-Decoder
  (`MakeTiledAstcDecoder`).

## Dungeon-Run und Karte (1.5.27)
- Run-Signal: `PlayerFarming.Instance` gesetzt und `PlayerFarming.Location` (statisch @0x20) in
  `GameManager.IsDungeon` (ohne None): 7–10, 51, 53, 81, 86, 93, 96.
- Herzen live in `Health` (Basis von HealthPlayer): _totalHP 0xAC, _HP 0xB0, Blau 0xB4,
  Schwarz 0xB8, Feuer 0xBC, Eis 0xC0, Geist gesamt 0xC4, Geist 0xC8 (halbe Herzen).
- PlayerFarming: _currentWeapon 0x350, _currentCurse 0x354 (None = 9999), currentWeaponLevel
  0x358, currentCurseLevel 0x360, currentRelicType 0x368, RunTrinkets 0x68.
- Tarot-Name: `TarotCards.LocalisedName(Card, upgradeIndex)` = `TarotCards/{Card}/Name` + "+" je
  Upgrade; Wirkung `TarotCards/{Card}/Description`.
- `BiomeGenerator_TypeInfo` main+0x7139CB8, Instance statisch @0x18; Rooms 0x88, CurrentX/Y
  0x208/0x20C. BiomeRoom: x 0x20, y 0x24, N/E/S/W_Room 0x38/0x40/0x48/0x50 (RoomConnection:
  Room 0x10, ConnectionType 0x18, False = 0), Discovered 0x5A, Visited 0x5C, IsRespawnRoom 0x70,
  IsDeathCatRoom 0x71, IsYngyaRoom 0x72, IsMysticShopRoom 0x73, IsBoss 0x74.
- Spielkarte (`MiniMap.OnChangeRoom`): Visited → voll; sonst bei HideUnexplored nur Discovered
  blass, Rest als Fragezeichen. Der Companion zeigt nur Visited/Discovered.

## Kult-Tab (1.5.27)
- DataManager: CurrentCultLevel 0x540 (CultLevel One = 0), CultName 0x5D0 (leer, bis der Kult über
  die Follower-Quest benannt wurde), BaseStructures 0xD18 (List<StructuresData>).
- StructuresData: Type 0x10, Inventory 0x98 (List<InventoryItem>).
- Silos: `Structures_SiloSeed.get_Capacity` = 250 bei Typ 334 (SEED_BUCKET), sonst 15;
  `Structures_SiloFertiliser.get_Capacity` = 250 bei Typ 333 (POOP_BUCKET), sonst 15.
  Saat-Silos: Typen 169/334, Dünger-Silos: 170/333. Inhalt = Summe der Inventarmengen.
- Offen: Schmutz (Kot/Erbrochenes) nicht umgesetzt; die Gebäudetypen dafür sind noch zu
  bestimmen (optional laut Spec).

## Thor-Test 5 (0.13.0)

- **Spielschrift.** Die eingebaute Host-Schrift ist 3×5 px, nur A–Z (Umlaute werden zu A/O/U
  gefaltet, `'` zeichnet leer). Das Modul liefert stattdessen die TMP-Schrift `LaptureRegular`
  (Bundle `6685d1be…`, 169 Zeichen U+0020–U+2026, SDF-Atlas 1024², Padding 11, Versallinie 67) über
  die Font-Erweiterung (`decode_font`, Paketdatei `cotl_font.txt`) und `module:cotl:font`. Der Atlas
  wird auf 30 px Versalhöhe umgerechnet (bilinear, Kante d = 0,5) und als 25 px gemeldet: der Host
  tastet nächstliegend ab und skaliert auf text_scale × 5 px, die Spielschrift erscheint so 20 %
  größer. Zeilenabstand mehrzeiliger Labels: line_gap 14 (Stufe 4) bzw. 18 (Stufe 5).
- **GIVE_ITEM** (Objectives.TYPES 33, z. B. „Seeking Sozo“): Objectives_GiveItem.TargetType @0x88
  zählt wie COLLECT_ITEM für das Quest-Symbol im Inventar.
- **Kartentüren** wie `MiniMapIcon.SetIcons`: Stummel je Richtung, wenn `RoomConnection.get_Connected`
  (Typ True 1, Boss 4 oder ≥ NextLayer 6), der Zielraum nicht `Hidden` (BiomeRoom 0x5B) ist und es
  nicht NextLayer nach Norden ist. Exit 3, Entrance 2, DoorRoom 5 zeichnet die Spielkarte nicht – sie
  hatten vorher Scheinverbindungen erzeugt. Sichtbarkeit wie `MiniMap.OnChangeRoom`: besucht → voll,
  nur entdeckt → halbe Deckkraft. Raumform: x = x·(IconSize.x + Spacing), y = y·(IconSize.y +
  Spacing) (MiniMap 0x78/0x7C/0x80, Werte nur zur Laufzeit; das Prefab liegt nicht in den Bundles).
  Der Companion nutzt 8 : 5 mit halber Raumhöhe Abstand – Annahme, am Thor zu prüfen.
- **Waffen/Flüche/Reliquien**: WeaponData/CurseData/RelicData-MonoBehaviours in resources.assets
  (m_Script 3987/372/2731 in globalgamemanagers.assets), UISprite → Atlas `WeaponsIcons` bzw.
  `_Atlas_Relics` (ASTC).
- **Spine-Bilder** (Bundle `4cccbc…`, MonoBehaviour `<name>_Atlas` → TextAsset `.atlas` + Material
  `_MainTex`): HUD-Herzen aus `heart.atlas` (HeartFull[_Blue|_Black|_Fire|_Ice], …HalfLeft,
  HeartBack, HeartSpiritOutline), Tarotkarten aus `WeaponCard.atlas`. `rotate: true` = im Atlas
  gegen den Uhrzeigersinn abgelegt (Original (x, y) bei (y, w−1−x)). Die WeaponCard-Textur ist
  4096×1024, die Atlasseite 8192×2048: Regionen werden skaliert. Karte → Region über
  `TarotCards.Skin` (Sprungtabelle @0x6C5DEC8, String-Literale) und die Skins in `WeaponCard.skel`
  (Spine 3.8, Slot „Card Front“; `tools/re/spine_skins.py`, `tools/re/tarot_regions.py`).
- **Dreck**: BaseStructures-Typen POOP 97 und Varianten 337–342, 419; VOMIT 48; DEAD_WORSHIPPER 47.

## Dungeon-Karte wie im Spiel (0.14.0)

- **Raumsymbole**: `MiniMapIcon.SetIcons` zeigt `GenerateRoom.MapIcon` (0x40) des Raum-Prefabs.
  `BiomeRoom.GameObjectPath` (0x68) ist der Addressables-Pfad ("Assets/_Rooms/….prefab"); die
  Tabelle Prefab → Sprite (`native/room_icons_1_5_27.inc`, `tools/re/room_icons.py`) stammt aus den
  GenerateRoom-MonoBehaviours der 265 Raum-Bundles; die Sprites (Icon_Boss, Icon_Entrance,
  Icon_WeaponShop, Icon_Tarot, …) liegen im MapIcons-Bundle `f2a159f3…` (mit TypeTree). Die alten
  Farbmarken (IsBoss/IsMysticShopRoom/IsRespawnRoom) zeigte die Spielkarte nicht und sind entfernt.
- **Fragezeichen** wie `MiniMapIcon.SetSelfToQuestionMark`: unentdeckter Raum (nicht Startraum,
  nicht Hidden) mit verbundener Tür zu einem besuchten Raum, dessen Gegentür zurückzeigt. Ohne
  Raumbild und Türen.
- **Spielerfigur**: Das Kartensymbol von `MiniMap.CurrentRoom` sitzt im MiniMap-Prefab (Szene, nicht
  lesbar). Ersatz: `Coop_Indicator_Lamb` (resources.assets; Atlas `Coop_Indicator`, Textur in
  `a74de6af…`, über `texture.m_FileID` = 1 – Tabelle externer Atlastexturen in icons.cpp).
  `Map_You` ist ein Raumrahmen, `MapPlayer` eine Raumkachel.

## Herzen (0.17.0)

`HealthPlayer` überschreibt die Getter von `Health` und liest eigene Felder: PLAYER_BLACK_HEARTS
0x3E8, PLAYER_BLUE_HEARTS 0x3EC, PLAYER_FIRE_HEARTS 0x3F0, PLAYER_ICE_HEARTS 0x3F4,
PLAYER_SPIRIT_HEARTS 0x3F8, PLAYER_HEALTH 0x3FC, PLAYER_TOTAL_HEALTH 0x400. Nur
`get_TotalSpiritHearts` liest das Basisfeld 0xC4. Die Basisfelder (`_BlueHearts` usw.) bleiben auf
Standardwerten, deshalb zeigte der Companion vorher nur rote Herzen.

## Quest-Texte (0.18.1)

Zusätzlich beschrieben nach `get_Text` der Unterklassen: PLACE_STRUCTURES (DecoType @0x7C: 1 Woolhaven,
2 Rot, 3 Ewefall, 4 Yewbark, sonst PlaceDecorations; Count @0x80, Target @0x84), GIVE_ITEM,
DEFEAT_KNUCKLEBONES, TALK_TO_FOLLOWER, EAT_MEAL, DRINK, MATING, USE_RELIC, FINISH_RACE, FLOWER_BASKETS,
BUILD_WINTER_DECORATION. Noch mit Typname: KILL_ENEMIES, FIND_FOLLOWER, ASSIGN/CRAFT_CLOTHING, GET/FEED/
WALK_ANIMAL, SHOW_FLEECE, LEGENDARY_*, FIND_CHILDREN, WIN_FLOCKADE_BET, BLIZZARD_OFFERING, SHOOT_DUMMIES,
RECRUIT_CURSED_FOLLOWER.

## Symbole in Texten, weitere Quest-Texte (0.19.0)

`<sprite name="icon_…">` in Tarot- und Gegenstandstexten: `TextWithIcons` setzt Herzen als eigene
Schriftzeichen U+1E00–U+1E03 (Umriss aus heart.atlas, `AddIconGlyphs`) mit Farb-Span, Labels brauchen
`color_markup`. Der Host zeichnet Glyphen einfarbig. icon_blackSoul und icon_SinDoctrine entfallen,
der Text nennt Fervour bzw. Sin. Quest-Daten nur bei Bedarf (`ReadQuestData`): EnemiesKilled 0x11B0,
BlizzardOfferings 0xB90, Tiere 0xB68/0xB78/0xB80 (Type 0x10, ID 0x18, GivenName 0x60), Waffen 0xEE0,
Schwert-Tag 0x254. Ohne eigenen Text bleiben: DEPOSIT_FOOD und NO_HEALING (im Spiel ungenutzt),
FIND_RELIC, STARVING_FOLLOWER, LOW_FAITH_FOLLOWER (keine Unterklasse), Objectives_Story.

## Abschlussprüfung (0.20.0)

Titelbild: `UIMainMenuController.Instance` (TypeInfo 0x71428F8, statisch @0x0), `OnDestroy` setzt es
auf null; headless gesehen: gesetzt am Titel, null nach dem Laden. `SaveAndLoad.Loaded` und der alte
DataManager bleiben nach „Zum Hauptmenü“ stehen, daher hat dieses Signal Vorrang.
Bereit-Signal: `SaveAndLoad.Loaded` (TypeInfo 0x7140C48, statisch @0x4), ersatzweise Follower oder Tag > 1;
am Titelbild existiert bereits ein DataManager mit Standardwerten. Werte: `RecordingPublisher` hält sie
je Lesestand vor (Neuaufbau bei neuem Stand oder Auswahl) und gibt nur die der sichtbaren Seite weiter,
nach Seitenwechsel 30 Ticks alle (Überblendung). Profil (EDEN_DSMOD_PROFILE, PC, geladener Stand):
165 µs → 37 µs je Tick. Tarot-Fenster liegt im Modul (Aktionen run.select/run.close), da Flags nur das
Paket setzt und das Fenster mit dem Run schließen muss.

## Ausrüstungskarten (WeaponInfoCard, CurseInfoCard, RelicInfoCard)
- Tabellen: `EquipmentManager` (TypeInfo 0x713B9A0) statisch WeaponsData @0x8, CursesData @0x10,
  relicData @0x18; Il2Cpp-Arrays, erst beim ersten `GetWeaponData`/`GetCurseData`/`GetRelicData`
  geladen. Headless in der Basis ohne Ausrüstung: Waffen und Flüche null, Reliquien gesetzt.
- Waffe: Mittel aus `WeaponCombos.Damage` (@0x14) × (1 + Tarot + DM@0x1024 + Vlies + 0.07·Stufe + 0.13
  + 0.2 bei PlayerRelic.PlayerScaleModifier > 1 + 1 bei DoubleDamage + DamageMultiplier)
  × DifficultyData@0x2C, auf eine Stelle gerundet. Tempo: WeaponData.Speed (@0x4C).
- Fluch: CurseData.Damage (@0x6C) × (Vlies + Tarot + DM@0x1024 + Stufe/6 + 1 bei DoubleDamage
  + DamageMultiplier + 1 bei Sword_Ratau + Eisfluch-Bonus). Vlies: 1 → statisch @0x38, 6 → 0.75, sonst 1.
- Tarot: Sun 0.2 außer nachts, Moon 0.3 nachts (DataManager.CurrentPhase @0x614, Night = 4),
  IncreasedDamage 0.2/0.5, CorruptedFullCorruption 3; Potion 0.25/0.5/1 (Fluch); CursedIce 3 (Eisfluch).
  Nicht nachgebildet: Coop-Karten und `IsCorruptedPositiveEffectNegated`.
- Reliquie: Aufladung nach DamageRequiredToCharge (@0x28): < 50 Fast, < 80 Average, sonst Slow.
  Texte `Relics/{Typ}/Description`, `/Lore`; Abwandlungen ohne eigenen Text nehmen die Grundform.

