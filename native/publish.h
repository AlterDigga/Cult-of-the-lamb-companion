// Überträgt GameSnapshot-Inhalte als veröffentlichte Werte (Namensschema im Plan).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "snapshot.h"
#include "terms.h"

struct EdenDsmodHostApi;

namespace cotl {

struct Publisher {
    virtual ~Publisher() = default;
    virtual void I64(const std::string& name, int64_t v) = 0;
    virtual void F64(const std::string& name, double v) = 0;
    virtual void Text(const std::string& name, std::string_view v) = 0;
};

class HostPublisher : public Publisher {
public:
    explicit HostPublisher(const EdenDsmodHostApi* host) : host_(host) {}
    void I64(const std::string& name, int64_t v) override;
    void F64(const std::string& name, double v) override;
    void Text(const std::string& name, std::string_view v) override;

private:
    const EdenDsmodHostApi* host_;
};

// Werte je Seite: hält die Werte eines Lesestands vor. Der Host löscht sie jeden Tick; neu
// aufgebaut werden muss nur, wenn sich der Stand ändert, und nur die sichtbare Seite braucht ihre.
enum class PageGroup : uint8_t { Always, Followers, Problems, Quests, Items, Dungeon };
PageGroup GroupOfValue(std::string_view name);
std::optional<PageGroup> GroupOfPage(std::string_view page); // nullopt: unbekannt, alles zeigen

class RecordingPublisher : public Publisher {
public:
    void I64(const std::string& name, int64_t v) override;
    void F64(const std::string& name, double v) override;
    void Text(const std::string& name, std::string_view v) override;
    void Clear() { entries_.clear(); }
    // page nullopt: alle Werte; sonst Always und die Werte dieser Seite
    void Replay(Publisher& out, std::optional<PageGroup> page) const;

private:
    enum class Kind : uint8_t { Int, Float, Str };
    struct Entry {
        std::string name;
        PageGroup group;
        Kind kind;
        int64_t i;
        double f;
        std::string s;
    };
    std::vector<Entry> entries_;
};

// Hält ein Fenster beim angetippten Eintrag, auch wenn sich die Zeilen umsortieren: beim Antippen
// wird die Zeile (flag) in einen Schlüssel übersetzt, -1 = zu.
template <class Key>
class KeepSelection {
public:
    template <class KeyAt>
    const std::optional<Key>& Update(int64_t flag, KeyAt key_at) {
        if (flag != flag_) {
            flag_ = flag;
            key_ = flag < 0 ? std::nullopt : std::optional<Key>(key_at(flag));
        }
        return key_;
    }
    const std::optional<Key>& Current() const { return key_; }

private:
    int64_t flag_ = -1;
    std::optional<Key> key_;
};

// Zeile der Follower-Liste ↔ Follower-ID; Inventar-Auswahl (g * 1000 + i) ↔ ITEM_TYPE. Für
// KeepSelection: Key_at liefert 0-Fall über optional, daher value_or(-1) beim Aufrufer.
std::optional<int32_t> FollowerIdAtRow(const GameSnapshot& s, int64_t row);
int64_t FollowerRowOf(const GameSnapshot& s, int32_t id);
std::optional<int32_t> InventoryTypeAt(const GameSnapshot& s, int64_t selected);
int64_t InventoryRowOf(const GameSnapshot& s, int32_t item_type);

// fol.count, fol.name{i}, fol.level{i}, fol.role{i}, fol.sat{i}, fol.loyal{i}, fol.ill{i},
// fol.happy{i}, fol.group{i}. Reihenfolge: normale Follower, dann Missionare,
// dann Gefangene; innerhalb der Gruppe wie im Spiel.
void PublishFollowers(Publisher& p, const GameSnapshot& s, const Terms& terms);

// Gruppen wie im Spiel-Menü (0 Currency, 1 Food, 2 Items), Reihenfolge der Menü-Listen; nur Mengen > 0
// und nur Gegenstände, die das Menü zeigt. inv.count (gesamt), inv.n{g}, inv.g{g}.name{i}/qty{i}/
// icon{i} ("module:cotl:item/<ITEM_TYPE>")/quest{i} (1 = offene Sammel-Quest braucht ihn).
// `selected` = g * 1000 + i: inv.sel_name, inv.sel_desc, inv.sel_qty, inv.sel_icon.
void PublishInventory(Publisher& p, const GameSnapshot& s, const Terms& terms, int64_t selected);

// g.in_run (1 während eines Runs), run.heart_count und
// run.heart{i} (Bildschlüssel module:cotl:heart/<art>_<full|half|empty> in HUD-Reihenfolge),
// run.weapon_icon/curse_icon (module:cotl:equip/<EquipmentType>), run.relic_icon
// (module:cotl:relic/<RelicType>), run.has_weapon,
// run.weapon/curse/relic (Name, leer ohne), run.weapon_lvl, run.curse_lvl, run.has_curse,
// run.has_relic, run.tarot_count, run.tarot_name{i} (Name + "+" je Upgrade wie im Spiel),
// run.tarot_icon{i}. `selected` = Index der angetippten Karte: run.sel_open, run.sel_name, run.sel_text.
void PublishRun(Publisher& p, const GameSnapshot& s, const Terms& terms, int64_t selected);

// g.loaded (Spielstand geladen), cult.name, cult.level (1-basiert wie im Spiel), cult.seed/seed_cap/has_seed und
// cult.fert/fert_cap/has_fert (Silo-Inhalt und -Kapazität, has_* = mindestens ein Silo).
void PublishCult(Publisher& p, const GameSnapshot& s);

// Auswahl im Run neben den Tarot-Indizes
inline constexpr int64_t kRunSelWeapon = 100, kRunSelCurse = 101, kRunSelRelic = 102;

// Kartenfläche auf der Dungeon-Seite (tools/gen_manifest.py, MAP_W/MAP_H)
inline constexpr int32_t kMapAreaW = 1160;
inline constexpr int32_t kMapAreaH = 380;

// Dungeon-Karte (BuildMap): map.sz (Index in kMapSizes), map.count, map.x{i}/y{i} (px in der
// Kartenfläche), map.cur{i} (aktuell), map.vis{i} (besucht, nicht aktuell), map.dim{i} (nur
// entdeckt), map.q{i} (Fragezeichen), map.has_icon{i}/map.icon{i} (Raumsymbol
// module:cotl:mapicon/<Sprite>); Zähler zusätzlich je Größe (map.count<sz>, map.hcount<sz>, …);
// Türstummel getrennt nach Richtung:
// map.hcount/hx{i}/hy{i}/hdim{i} (Osten/Westen), map.vcount/vx{i}/vy{i}/vdim{i} (Norden/Süden).
// Ohne Run: alle Zähler 0.
void PublishMap(Publisher& p, const GameSnapshot& s);

// fol.sel_name, fol.sel_detail für Zeile `index` der Follower-Liste (gleiche Reihenfolge wie
// PublishFollowers). Außerhalb der Liste wird nichts veröffentlicht.
void PublishSelectedFollower(Publisher& p, const GameSnapshot& s, const Terms& terms,
                             int64_t index);

} // namespace cotl
