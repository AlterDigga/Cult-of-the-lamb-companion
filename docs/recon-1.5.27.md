# Cult of the Lamb 1.5.27 – Analyse

Stand: 2026-10-31. Quelle: Il2CppDumper-Ausgabe in `work/il2cpp/` (nicht versioniert).

## Spiel
- Title-ID: `01002E7016C46000`, Update `01002E7016C46800` v1900544 = Anzeigeversion **1.5.27**
- Build-ID (main): `316A4D4881A9F1E077A07262E917917C` → Datendatei `316A4D4881A9F1E0.json`
- **1.5.28** (Update v1966080): dieselbe Build-ID, `main`, Metadaten, `resources.assets`,
  `globalgamemanagers` und alle Bundles bytegleich; dieselben 10.443 Dateien. Unterschiede: `main.npdm`
  (510 Byte ab dem Signaturbereich) und vier vom Companion ungenutzte Dateien (`mscorlib.dll-resources.dat`,
  `System.Data.dll-resources.dat`, `unity default resources`, `resources.resource`; 1.5.27 mit hactool,
  1.5.28 mit Eden entpackt, Unterschiede können auch daher kommen). Kein Port nötig, Paket 0.21.1
  headless auf 1.5.28 geprüft. Ein Update mit anderer Build-ID braucht dagegen neue Offsets.
- Entpacken ab 1.5.28: hactool bricht ab („Invalid BKTR layout!“). Stattdessen `eden-cli --install <Update>`,
  dann `EDEN_DUMP_PATCHED=1 eden-cli --dump-romfs <Ziel> <Basis-NSP>`. `EDEN_DUMP_PATCHED` ist eine lokale,
  nicht eingecheckte Änderung an `~/cotl-deps/eden-duo/src/yuzu_cmd/dsmod_harness.cpp` (PatchManager).
- Unity 2022.3.62f3, IL2CPP-Metadaten v31, SDK 20.5.19
- DLCs: `01002E7016C47001`–`…47005`

## Einstiegspunkte (Singletons)
Offset von `static_fields` in `Il2CppClass` ist noch live zu prüfen.

| TypeInfo | Adresse | statisches Feld |
|---|---|---|
| `DataManager_TypeInfo` | `main+0x713AC50` | `instance` @ 0xE0 |
| `PlayerFarming_TypeInfo` | `main+0x713FE88` | `Instance` @ 0x10, `players` @ 0x0 |
| `CultFaithManager_TypeInfo` | `main+0x713AA00` | – |
| `MMBiomeGeneration.BiomeGenerator_TypeInfo` | in `script.json` | `Instance` @ 0x18 |

## DataManager (Auswahl)
| Feld | Offset | Typ |
|---|---|---|
| CultFaith | 0x354 | float |
| HungerBarCount | 0x340 | float |
| IllnessBarCount / IllnessBarDynamicMax | 0x344 / 0x348 | float |
| CurrentDayIndex | 0x610 | int |
| CultName | 0x5D0 | string |
| CurrentCultLevel | 0x540 | enum |
| ShrineLevel / TempleLevel | 0x3BC / 0x3C0 | int |
| PlayerFleece | 0x11C | int |
| Followers | 0xCE0 | List&lt;FollowerInfo&gt; |
| Followers_Dead | 0xCF0 | List&lt;FollowerInfo&gt; |
| Followers_Elderly_IDs / _Imprisoned_IDs / _OnMissionary_IDs | 0xDD0 / 0xDC8 / 0xDD8 | List&lt;int&gt; |
| items (Inventar Basis) | 0xE80 | List&lt;InventoryItem&gt; |
| CurrentRunWeaponLevel / CurrentRunCurseLevel | 0xF28 / 0xF30 | int |
| dungeonRun / dungeonRunDuration | 0x84 / 0x88 | int / float |

## FollowerInfo (Auswahl)
ID 0x10, _name 0x18 (string), Age 0xB4, LifeExpectancy 0xB8, XPLevel 0x12C, Adoration 0x128,
Traits 0x1E8 (List&lt;TraitType&gt;), OldAge 0x1F4, FollowerRole 0x104, CursedState 0x20 (Thought),
_HP 0x218, _maxHP 0x214, _happiness 0x224, _faith 0x228, _satiation 0x230, _starvation 0x234,
_bathroom 0x240, _social 0x248, _rest 0x250, _illness 0x254, _injured 0x258, _exhaustion 0x25C,
_dissent 0x280, _freezing 0x264, _overheating 0x268

## InventoryItem
type 0x10 (int, ITEM_TYPE), quantity 0x14, QuantityReserved 0x18

## Dungeon-Run
- PlayerFarming: RunTrinkets 0x68 (List&lt;TarotCard&gt;: CardType 0x10, UpgradeIndex 0x14),
  health 0x1C0 (HealthPlayer), _currentWeapon 0x350, _currentCurse 0x354,
  currentWeaponLevel 0x358, currentCurseLevel 0x360, _playerRelic 0x398
- HealthPlayer: PLAYER_HEALTH 0x3FC, PLAYER_TOTAL_HEALTH 0x400, BLUE 0x3EC, BLACK 0x3E8,
  SPIRIT 0x3F8 / SPIRIT_TOTAL 0x408, FIRE 0x3F0, ICE 0x3F4
- BiomeGenerator: Rooms 0x88 (List&lt;BiomeRoom&gt;), CurrentX/Y 0x208/0x20C, CurrentRoom 0x248,
  RoomEntrance 0x1A8, TotalFloors 0x20
- BiomeRoom: x 0x20, y 0x24, N/E/S/W_Room 0x38–0x50, Completed 0x59, Discovered 0x5A,
  Hidden 0x5B, Visited 0x5C, IsBoss 0x74, IsMysticShopRoom 0x73, IsRespawnRoom 0x70

## Texte
I2 Localization, Quelle `I2Languages` in `romfs:/Data/resources.assets`.
Begriffe z. B. `Traits/…`, `Inventory/…`, `Curses/…`, `FollowerNames/…`.
Follower-Namen stehen schon fertig als String in `FollowerInfo._name`.

## Spielstand
`slot_0MP` / `meta_0MP`: Header `MP`, MessagePack (vermutlich LZ4-komprimiert).
`settings` / `persistence`: Header `ZB` + gzip.
