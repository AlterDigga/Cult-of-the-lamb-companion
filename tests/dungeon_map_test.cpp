#include <cstdlib>
#include <iterator>

#include "dungeon_map.h"
#include "test_util.h"

using cotl::BuildMap;
using cotl::kDoorE;
using cotl::kDoorN;
using cotl::kDoorS;
using cotl::kDoorW;
using cotl::Room;
using cotl::RoomKind;

namespace {

Room R(int32_t x, int32_t y, bool discovered, bool visited, std::array<bool, 4> doors = {},
       RoomKind kind = RoomKind::Normal) {
    Room r;
    r.x = x;
    r.y = y;
    r.discovered = discovered;
    r.visited = visited;
    r.doors = doors;
    r.kind = kind;
    return r;
}

} // namespace

TEST(map_hides_undiscovered_rooms) {
    const std::vector<Room> rooms = {R(0, 0, true, true), R(1, 0, true, false), R(2, 0, false, false)};
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.cells.size() == 2);
    CHECK_EQ(m.cells[0].state, 3); // aktuell
    CHECK_EQ(m.cells[1].state, 1); // nur entdeckt
}

TEST(map_marks_current_and_visited_rooms) {
    const std::vector<Room> rooms = {R(0, 0, true, true), R(0, 1, true, true)};
    const auto m = BuildMap(rooms, 0, 1, 1000, 400);
    REQUIRE(m.cells.size() == 2);
    CHECK_EQ(m.cells[0].state, 2);
    CHECK_EQ(m.cells[1].state, 3);
    // Norden oben: (0,1) liegt über (0,0)
    CHECK(m.cells[1].py < m.cells[0].py);
}

TEST(map_rooms_are_wider_than_tall) {
    const auto m = BuildMap({R(0, 0, true, true)}, 0, 0, 1000, 400);
    CHECK(m.room_w > m.room_h);
    CHECK_EQ(m.room_w, cotl::kMapSizes[m.size_index] * 8 / 5);
    CHECK_EQ(m.room_h, cotl::kMapSizes[m.size_index]);
}

TEST(map_boss_kind_only_when_visible) {
    const std::vector<Room> hidden = {R(0, 0, true, true), R(1, 0, false, false, {}, RoomKind::Boss)};
    const auto a = BuildMap(hidden, 0, 0, 1000, 400);
    REQUIRE(a.cells.size() == 1);
    CHECK(a.cells[0].kind == RoomKind::Normal);
    const std::vector<Room> seen = {R(0, 0, true, true), R(1, 0, true, false, {}, RoomKind::Boss)};
    const auto b = BuildMap(seen, 0, 0, 1000, 400);
    REQUIRE(b.cells.size() == 2);
    CHECK(b.cells[1].kind == RoomKind::Boss);
}

TEST(map_shifts_negative_coordinates_into_area) {
    const std::vector<Room> rooms = {R(-3, -2, true, true), R(4, 2, true, true)};
    const auto m = BuildMap(rooms, -3, -2, 1400, 600);
    REQUIRE(m.cells.size() == 2);
    for (const auto& c : m.cells) {
        CHECK(c.px >= 0 && c.py >= 0);
        CHECK(c.px + m.room_w <= 1400 && c.py + m.room_h <= 600);
    }
}

TEST(map_picks_largest_size_that_fits) {
    CHECK_EQ(BuildMap({R(0, 0, true, true), R(1, 0, true, true)}, 0, 0, 1000, 400).size_index, 0);
    // 6 Räume breit: eine kleinere Größe passt in 1000 px
    std::vector<Room> wide;
    for (int32_t x = 0; x < 6; ++x) wide.push_back(R(x, 0, true, true));
    const auto m = BuildMap(wide, 0, 0, 1000, 400);
    CHECK(m.size_index > 0);
    CHECK(m.cells.front().px >= 0 && m.cells.back().px + m.room_w <= 1000);
}

// Große Etage: nie kleiner als die Mindestgröße, stattdessen folgt die Karte dem aktuellen Raum
TEST(map_big_floor_keeps_minimum_size_and_follows_current_room) {
    std::vector<Room> wide;
    for (int32_t x = 0; x < 30; ++x) wide.push_back(R(x, 0, true, true));
    const auto m = BuildMap(wide, 15, 0, 1000, 400);
    CHECK_EQ(m.size_index, static_cast<int32_t>(std::size(cotl::kMapSizes)) - 1);
    const auto& cur = m.cells[15];
    CHECK_EQ(cur.state, 3);
    CHECK(std::abs(cur.px + m.room_w / 2 - 500) <= 1); // waagrecht mittig
    CHECK(m.cells.front().px < 0);                       // Rest ragt über den Rand
    CHECK(m.cells.back().px + m.room_w > 1000);
    // Senkrecht passt die Etage: dort weiter mittig wie bisher
    CHECK(std::abs(cur.py + m.room_h / 2 - 200) <= 1);
}

// Am Rand der Etage kein leerer Streifen: die Karte schließt bündig mit der Fläche ab
TEST(map_big_floor_clamps_at_floor_edge) {
    std::vector<Room> wide;
    for (int32_t x = 0; x < 30; ++x) wide.push_back(R(x, 0, true, true));
    const auto left = BuildMap(wide, 0, 0, 1000, 400);
    CHECK_EQ(left.cells.front().px, cotl::MapGap(left.size_index));
    const auto right = BuildMap(wide, 29, 0, 1000, 400);
    CHECK_EQ(right.cells.back().px + right.room_w, 1000 - cotl::MapGap(right.size_index));
}

TEST(map_door_stub_points_out_of_room) {
    // Tür nach Osten in einen unentdeckten Raum: Stummel trotzdem sichtbar
    const std::vector<Room> rooms = {R(0, 0, true, true, {false, true, false, false}),
                                     R(1, 0, false, false)};
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.cells.size() == 1);
    REQUIRE(m.hstubs.size() == 1);
    CHECK_EQ(m.vstubs.size(), size_t{0});
    const auto& c = m.cells[0];
    const auto& s = m.hstubs[0];
    CHECK_EQ(s.px, c.px + m.room_w);
    CHECK(s.py > c.py && s.py + m.stub_t < c.py + m.room_h);
    CHECK(!s.dim);
}

TEST(map_neighbouring_stubs_meet_in_the_gap) {
    const std::vector<Room> rooms = {R(0, 0, true, true, {true, false, false, false}),
                                     R(0, 1, true, false, {false, false, true, false})};
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.vstubs.size() == 2);
    const auto& lower = m.cells[0]; // (0,0) liegt unten
    const auto& upper = m.cells[1];
    // Stummel nach Norden endet dort, wo der nach Süden des oberen Raums beginnt (±1 px)
    const auto& up = m.vstubs[0].py < m.vstubs[1].py ? m.vstubs[1] : m.vstubs[0];
    const auto& down = m.vstubs[0].py < m.vstubs[1].py ? m.vstubs[0] : m.vstubs[1];
    CHECK_EQ(up.py + m.stub_len, lower.py);
    CHECK_EQ(down.py, upper.py + m.room_h);
    CHECK(down.py + m.stub_len >= up.py);
    CHECK(down.dim); // gehört zum nur entdeckten Raum
}

TEST(map_west_and_south_stubs) {
    const auto m = BuildMap({R(0, 0, true, true, {false, false, true, true})}, 0, 0, 1000, 400);
    REQUIRE(m.hstubs.size() == 1 && m.vstubs.size() == 1);
    const auto& c = m.cells[0];
    CHECK_EQ(m.hstubs[0].px + m.stub_len, c.px);
    CHECK_EQ(m.vstubs[0].py, c.py + m.room_h);
    (void)kDoorN, (void)kDoorE, (void)kDoorS, (void)kDoorW;
}

TEST(map_question_rooms_are_cells_without_room) {
    std::vector<Room> rooms = {R(0, 0, true, true), R(1, 0, false, false), R(2, 0, false, false)};
    rooms[1].question = true;
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.cells.size() == 2);
    CHECK(m.cells[1].question);
    CHECK_EQ(m.cells[1].state, 0);
    CHECK(m.cells[1].px > m.cells[0].px);
}

// Fragezeichen-Räume sind kleiner und haben keine eigenen Türen: die Tür des besuchten Nachbarn
// reicht bis an die verkleinerte Kachel
TEST(map_door_reaches_question_room) {
    std::vector<Room> rooms = {R(0, 0, true, true, {true, true, false, false}),
                               R(1, 0, false, false), R(0, 1, false, false)};
    rooms[1].question = true;
    rooms[2].question = true;
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.cells.size() == 3);
    REQUIRE(m.hext.size() == 1 && m.vext.size() == 1);
    const auto& here = m.cells[0];
    const auto& east = m.cells[1];
    const auto& north = m.cells[2];
    // Osten: Verlängerung beginnt am Ende des Stummels und endet an der Kante der kleinen Kachel
    CHECK_EQ(m.hext[0].px, here.px + m.room_w + m.stub_len);
    CHECK_EQ(m.hext[0].px + m.ext_len, east.px + m.q_inset);
    // Norden: von der Kante der kleinen Kachel bis zum Anfang des Stummels
    CHECK_EQ(m.vext[0].py, north.py + m.room_h - m.q_inset);
    CHECK_EQ(m.vext[0].py + m.ext_len, here.py - m.stub_len);
}

TEST(map_room_icon_from_prefab_path) {
    std::vector<Room> rooms = {R(0, 0, true, true), R(1, 0, true, false), R(2, 0, false, false)};
    rooms[0].path = "Assets/_Rooms/Boss Room Dungeon 1_1.prefab";
    rooms[1].path = "Assets/_Rooms/Dungeon Biome Door Room 1.prefab";
    rooms[2].path = "Assets/_Rooms/Boss Room Dungeon 1_2.prefab";
    rooms[2].question = true; // Fragezeichen verrät das Symbol nicht
    const auto m = BuildMap(rooms, 0, 0, 1000, 400);
    REQUIRE(m.cells.size() == 3);
    CHECK_EQ(m.cells[0].icon, std::string("Icon_Boss"));
    CHECK_EQ(m.cells[1].icon, std::string("Icon_Entrance"));
    CHECK_EQ(m.cells[2].icon, std::string(""));
    CHECK_EQ(cotl::RoomIcon("Assets/_Rooms/Unknown.prefab"), std::string_view(""));
}

TEST(map_empty_rooms_give_empty_layout) {
    const auto m = BuildMap({}, 0, 0, 1000, 400);
    CHECK_EQ(m.cells.size(), size_t{0});
    CHECK_EQ(m.hstubs.size() + m.vstubs.size(), size_t{0});
}
