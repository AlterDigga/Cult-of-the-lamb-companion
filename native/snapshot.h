// Spielzustand als Momentaufnahme, gelesen aus dem Gastspeicher.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dungeon_map.h"

#include "guest.h"

namespace cotl {

enum class FollowerGroup : uint8_t { Normal = 0, Missionary = 1, Prisoner = 2 };

// FollowerInfo.Thoughts: Gedanke (Thought), Glaubens-Modifikator, Anzahl gestapelt
struct ThoughtEntry {
    int32_t type = 0;
    float modifier = 0;
    int32_t quantity = 1;
};

struct Follower {
    int32_t id = 0;
    std::string name;
    int32_t level = 0;
    int32_t age = 0;
    int32_t life_expectancy = 0;
    int32_t role = 0;
    int32_t cursed_state = 0;
    int32_t dwelling_id = 0;
    bool old_age = false;
    std::vector<int32_t> traits;
    std::vector<ThoughtEntry> thoughts;
    // Bedürfnisse 0–100 (FollowerInfo-Setter begrenzen auf [0, 100])
    float satiation = 0, starvation = 0, faith = 0, happiness = 0, illness = 0, injured = 0,
          adoration = 0, // Loyalität, 0–100 (FollowerBrainStats.get_MAX_ADORATION)
          exhaustion = 0, dissent = 0;
    FollowerGroup group = FollowerGroup::Normal;
};

struct Objective {
    int32_t type = 0;
    int32_t follower_id = -1;
    int32_t item_type = -1; // -1 = keine Sammel-Quest
    float expire = 0;        // ExpireTimestamp, Spielminuten seit Spielbeginn
    bool has_expiry = false; // QuestExpireDuration > 0
    std::string group_id;    // GroupId (+0x18), I2-Begriff des Titels
    std::string unique_group; // UniqueGroupID (+0x40): Unterpunkte eines Quests
    // Unterklassenfelder +0x78, +0x7C, +0x80, +0x84, +0x88 als int; Bedeutung je Type, siehe quests.cpp
    std::array<int32_t, 5> ints{};
    std::string str2; // zweiter String: @0x88 RECRUIT_CURSED FollowerName, @0x90 FIND_FOLLOWER Name
    std::string str78; // string @0x78 (BED_REST FollowerName, GIVE_ITEM NPCTerm, KNUCKLEBONES CharacterNameTerm)
    bool complete = false;
    bool failed = false;
};

// Werte für die Ausrüstungskarten; negativ: Spieldaten nicht gefunden
struct GearData {
    float weapon_avg = -1, weapon_speed = -1, curse_damage = -1, relic_charge = -1;
    float dm_bonus = 0, fleece_damage = 0, relic_mult = 0, relic_scale = 1, difficulty = 1;
    int32_t fleece = 0, phase = 0;
    bool relic_double = false;
};

// Dungeon-Run von Spieler 1 (PlayerFarming.Instance). Herzen in halben Herzen wie im Spiel.
struct Run {
    float hp_red = 0, hp_red_max = 0, hp_blue = 0, hp_black = 0, hp_spirit = 0, hp_spirit_max = 0,
          hp_fire = 0, hp_ice = 0;
    int32_t weapon = 0, weapon_lvl = 0, curse = 0, curse_lvl = 0, relic = 0;
    std::vector<std::pair<int32_t, int32_t>> tarot;
    std::vector<Room> rooms; // BiomeGenerator.Rooms der aktuellen Etage
    int32_t cur_x = 0, cur_y = 0;
    std::optional<GearData> gear; // nur bei offenem Ausrüstungsfenster gelesen
};

struct AnimalInfo {
    int32_t type = 0; // ITEM_TYPE
    int32_t id = 0;
    std::string name; // leer: ungetauft
};

struct GameSnapshot {
    bool save_loaded = false; // SaveAndLoad.Loaded: am Titelbild false, nach dem Beenden nicht
    bool at_title = false;    // UIMainMenuController.Instance gesetzt
    int32_t day = 0;  // DataManager.CurrentDayIndex
    float time = 0;   // DataManager.CurrentGameTime, Spielminuten im Tag (0–1200)
    std::vector<Follower> followers;
    std::vector<Objective> objectives;              // Task 6
    std::vector<int32_t> offered_quest_follower_ids; // Task 6
    std::vector<std::pair<int32_t, int32_t>> items; // Task 7: (ITEM_TYPE, Menge)
    // Nur gelesen, wenn eine offene Quest sie braucht (Quest-Texte)
    std::vector<std::pair<int32_t, int32_t>> enemies_killed; // (Enemy, Anzahl)
    std::vector<std::pair<int32_t, int32_t>> offerings;      // (ITEM_TYPE, Menge) BlizzardOfferings
    std::vector<AnimalInfo> animals;
    std::vector<int32_t> legendary_known; // EquipmentType
    int32_t sword_return_day = 0;
    std::string cult_name;  // DataManager.CultName
    int32_t cult_level = 0; // DataManager.CurrentCultLevel (One = 0)
    // Silos im Kult: Summe der Inhalte und Kapazitäten (Structures_Silo*.get_Capacity)
    int32_t seed_stored = 0, seed_capacity = 0;
    int32_t fertiliser_stored = 0, fertiliser_capacity = 0;
    // Dreck in der Basis (BaseStructures): Kot-Haufen, Erbrochenes, Leichen
    int32_t poop = 0, vomit = 0, corpses = 0;
    std::optional<Run> run; // nur während eines Dungeon-Runs
};

// GameManager.IsDungeon ohne None (-1): Orte, an denen ein Run läuft.
bool IsDungeonLocation(int32_t location);

// nullopt, solange DataManager.instance fehlt oder die Follower-Liste sich während des Lesens
// ändert (Torn-Read-Schutz).
// Gedanken nur für diesen Follower lesen (viele Lesezugriffe, nur das Thoughts-Fenster zeigt sie)
inline constexpr int32_t kAllThoughts = -2;
inline constexpr int32_t kNoThoughts = -1;
std::optional<GameSnapshot> ReadSnapshot(const Guest& g, uint64_t main_base,
                                         int32_t thoughts_for = kAllThoughts, bool read_gear = false);

} // namespace cotl
