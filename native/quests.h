// Alle Quests für den Quests-Tab: angebotene (Follower mit "!") und angenommene, auch Story- und
// NPC-Quests. Texte wie ObjectivesData.get_Text der jeweiligen Unterklasse, siehe docs/re-notes.md.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "publish.h"
#include "snapshot.h"
#include "terms.h"

namespace cotl {

// Ein Unterpunkt eines Quests (ein ObjectivesData)
struct QuestStep {
    std::string text;
    bool done = false;
    int32_t progress = -1; // -1 = unbekannt
    int32_t target = -1;
};

struct QuestView {
    std::string key;        // bleibt beim Umsortieren gleich (Auswahl im Fenster)
    std::string title;      // GroupId-Titel, z. B. "Follower Quest"
    std::string text;       // Aufgabe, z. B. "Collect Berry"
    std::string giver;      // Follower-Name oder leer
    int32_t progress = -1;  // -1 = unbekannt
    int32_t target = -1;
    std::string due;        // "2 days left", "due today" oder leer
    bool offered = false;   // noch nicht angenommen
    std::vector<QuestStep> steps; // alle Unterpunkte, offene zuerst
};

// Ersetzt {0}, {1}, … durch args. Ist ein Argument leer, wird eine Klammer "(…)" um seinen
// Platzhalter samt führendem Leerzeichen entfernt (z. B. unbekannter Fortschritt).
std::string FormatTerm(std::string_view fmt, const std::vector<std::string>& args);

std::vector<QuestView> BuildQuests(const GameSnapshot& s, const Terms& terms);

// quest.count, quest.title{i}, quest.text{i}, quest.giver{i}, quest.due{i}, quest.offered{i},
// quest.more{i}; für `selected`: quest.sel_title, quest.sel_text, quest.sel_giver,
// quest.sel_progress, quest.sel_due, quest.step_count, quest.step{i}, quest.step_done{i}
void PublishQuests(Publisher& p, const std::vector<QuestView>& quests, int64_t selected);

} // namespace cotl
