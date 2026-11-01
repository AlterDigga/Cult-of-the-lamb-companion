// Dungeon-Karte wie die Spielkarte (MiniMap.OnChangeRoom, HideUnexplored): besucht → voll, nur
// entdeckt → blass, sonst nicht gezeichnet. Räume als Rechtecke, Türen als Stummel je Richtung
// (MiniMapIcon.SetIcons); zwei Stummel benachbarter Räume ergeben den Gang. Siehe
// docs/re-notes.md (Dungeon-Karte).
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cotl {

enum class RoomKind : int32_t { Normal = 0, Boss = 1, Shop = 2, Start = 3 };

// Richtungen wie BiomeRoom.N_Room, E_Room, S_Room, W_Room
inline constexpr int kDoorN = 0, kDoorE = 1, kDoorS = 2, kDoorW = 3;

struct Room {
    int32_t x = 0, y = 0;
    bool discovered = false, visited = false;
    RoomKind kind = RoomKind::Normal;
    std::array<bool, 4> doors = {}; // Tür je Richtung, wie die Spielkarte sie zeigt
    std::string path;               // BiomeRoom.GameObjectPath (Raum-Prefab)
    bool question = false;          // unentdeckt, aber neben einem besuchten Raum: "?"
};

// MapIcon-Sprite des Raum-Prefabs (GenerateRoom.MapIcon, room_icons_1_5_27.inc), leer ohne
std::string_view RoomIcon(std::string_view prefab_path);

// state: 0 = Fragezeichen (kein Raum), 1 = nur entdeckt, 2 = besucht, 3 = aktueller Raum
struct MapCell {
    int32_t px = 0, py = 0;
    int32_t state = 0;
    RoomKind kind = RoomKind::Normal;
    bool question = false;
    std::string icon; // Sprite-Name im MapIcons-Atlas, leer ohne Symbol
};

// Türstummel: linke obere Ecke; dim = gehört zu einem nur entdeckten Raum
struct MapStub {
    int32_t px = 0, py = 0;
    bool dim = false;
};

// Raumhöhen in px, aus denen BuildMap die größte passende wählt. Je Größe: Breite = 8/5 der Höhe,
// Abstand = halbe Höhe. Das Manifest hat je Größe eigene Vorlagen (tools/gen_manifest.py,
// MAP_SIZES).
// Kleinste Größe 64: passt die Etage auch so nicht, zeigt die Karte einen Ausschnitt um den
// aktuellen Raum (Thor-Test: kleinere Räume waren schlecht lesbar).
inline constexpr int32_t kMapSizes[] = {80, 72, 64};

struct MapLayout {
    int32_t size_index = 0; // Index in kMapSizes
    int32_t room_w = 0, room_h = 0;
    int32_t stub_len = 0, stub_t = 0; // Länge (halber Abstand) und Stärke eines Türstummels
    std::vector<MapCell> cells;
    std::vector<MapStub> hstubs; // nach Osten/Westen: stub_len × stub_t
    std::vector<MapStub> vstubs; // nach Norden/Süden: stub_t × stub_len
    // Fragezeichen-Räume: Kachel um q_inset je Seite kleiner; eine Tür dorthin wird um ext_len
    // bis an die kleine Kachel verlängert (hext: ext_len × stub_t, vext: stub_t × ext_len)
    int32_t q_inset = 0, ext_len = 0;
    std::vector<MapStub> hext, vext;
};

// Abmessungen der Größe k (wie im Manifest)
inline int32_t MapRoomW(int32_t k) { return kMapSizes[k] * 8 / 5; }
inline int32_t MapRoomH(int32_t k) { return kMapSizes[k]; }
inline int32_t MapGap(int32_t k) { return kMapSizes[k] / 2; }
inline int32_t MapStubLen(int32_t k) { return (MapGap(k) + 1) / 2; }
inline int32_t MapStubT(int32_t k) { return kMapSizes[k] / 6 > 4 ? kMapSizes[k] / 6 : 4; }
inline int32_t MapQInset(int32_t k) { return MapRoomH(k) / 8 > 2 ? MapRoomH(k) / 8 : 2; }
inline int32_t MapExtLen(int32_t k) { return MapGap(k) - MapStubLen(k) + MapQInset(k); }

// Ordnet die sichtbaren Räume zentriert in einer Fläche area_w × area_h an (Ursprung oben links,
// Norden = größeres y oben). Passt die Etage selbst in der kleinsten Größe nicht, wird je Achse
// auf den aktuellen Raum zentriert, ohne über den Rand der Etage hinaus zu verschieben; Zellen
// können dann außerhalb der Fläche liegen (das Manifest schneidet sie ab).
MapLayout BuildMap(const std::vector<Room>& rooms, int32_t cur_x, int32_t cur_y, int32_t area_w,
                   int32_t area_h);

} // namespace cotl
