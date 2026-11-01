// Probleme und Quests pro Follower, abgeleitet aus dem Snapshot. Schwellen und Zustände aus dem
// Spielcode, siehe docs/re-notes.md (Probleme, Task 6).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "publish.h"
#include "snapshot.h"
#include "terms.h"

namespace cotl {

// Reihenfolge = Dringlichkeit (kleiner = dringender)
enum class ProblemKind : int32_t {
    Starving = 0,
    Ill = 1,
    Injured = 2,
    Hungry = 3,
    Cursed = 4, // Frieren, Überhitzen, Durchnässt, Brennend, Zombie
    Dissent = 5,
    Exhausted = 6,
    Old = 7,
};
inline constexpr int kProblemKinds = 8;

// FollowerInfo-Konstanten (dump.cs) und Thought-Werte von FollowerInfo.CursedState
inline constexpr float kHungerThreshold = 30;   // FollowerInfo.HUNGER_THRESHOLD
inline constexpr int32_t kThoughtDissenter = 66;
inline constexpr int32_t kThoughtIll = 68;
inline constexpr int32_t kThoughtStarving = 131; // BecomeStarving
inline constexpr int32_t kThoughtOldAge = 146;
inline constexpr int32_t kThoughtZombie = 181;
inline constexpr int32_t kThoughtInjured = 310;
inline constexpr int32_t kThoughtFreezing = 480;  // bis 483: Overheating, Soaking, Aflame
inline constexpr float kGameMinutesPerDay = 1200; // TimeManager.GAME_MINUTES_PER_DAY

struct ProblemRow {
    int32_t follower_id = 0;
    std::string name;
    int32_t level = 0;
    ProblemKind kind = ProblemKind::Old;
    std::string reason;
};

// Eine Zeile pro Follower mit seinem dringendsten Problem; weitere Probleme als "+N more" im
// Grund. Sortiert nach Dringlichkeit. Quests zählen nicht (eigener Quests-Tab).
// filter >= 0: nur Follower mit Problem dieser Art (ProblemKind), Zeile zeigt diese Art.
std::vector<ProblemRow> DeriveProblems(const GameSnapshot& s, const Terms& terms,
                                       int32_t filter = -1);

// prob.count, prob.n<kind> (Anzahl je Art, auch mehrfach pro Follower), prob.name{i},
// prob.level{i}, prob.kind{i}, prob.reason{i}
void PublishProblems(Publisher& p, const GameSnapshot& s, const std::vector<ProblemRow>& rows);

// Merkt sich beim Antippen einer Zeile den Follower: die Zeilen können sich umsortieren, solange
// das Fenster offen ist. flag = Zeilenindex (@flag:prob.open), -1 = zu.
class ProblemSelection {
public:
    // Follower des offenen Fensters, -1 wenn keines offen ist oder die Zeile nicht existiert
    int32_t Update(int64_t flag, const std::vector<ProblemRow>& rows);

private:
    int64_t flag_ = -1;
    int32_t follower_id_ = -1;
};

// Alle Probleme des Followers, nach Dringlichkeit: prob.sel_name, prob.sel_level, prob.sel_count,
// prob.sel_kind{i}, prob.sel_reason{i}. Ohne passenden Follower nur prob.sel_count = 0.
void PublishSelectedProblems(Publisher& p, const GameSnapshot& s, int32_t follower_id);

} // namespace cotl
