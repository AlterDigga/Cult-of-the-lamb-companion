#include "font.h"
#include "test_util.h"

namespace {

// Atlas 32×32 mit einem Glyphen 'I' (Rechteck x 10, y 6 von unten, 4×20); Alpha 255 im Glyphen,
// 0 außerhalb (ein hartes "SDF": innen d = 1, außen d = 0).
ce_unity::TmpFont MakeFont() {
    ce_unity::TmpFont f;
    f.cap_line = 20;
    f.padding = 4;
    f.atlas_w = f.atlas_h = 32;
    f.atlas_image.width = f.atlas_image.height = 32;
    f.atlas_image.rgba.assign(32 * 32 * 4, 0);
    for (int y = 32 - 6 - 20; y < 32 - 6; ++y) {
        for (int x = 10; x < 14; ++x) {
            f.atlas_image.rgba[(y * 32 + x) * 4 + 3] = 255;
        }
    }
    ce_unity::TmpGlyph space;
    space.advance = 8;
    ce_unity::TmpGlyph i;
    i.width = 4;
    i.height = 20;
    i.bearing_x = 2;
    i.bearing_y = 20;
    i.advance = 9;
    i.rect_x = 10;
    i.rect_y = 6;
    i.rect_w = 4;
    i.rect_h = 20;
    f.glyphs = {space, i};
    f.chars = {{0x20, 0}, {'I', 1}};
    return f;
}

uint8_t Alpha(const cotl::HostFont& h, const EdenDsmodFontGlyph& g, int x, int y) {
    return h.atlas.rgba[((g.y + y) * h.atlas.width + g.x + x) * 4 + 3];
}

} // namespace

TEST(font_dense_run_from_lowest_codepoint) {
    const auto h = cotl::BuildHostFont(MakeFont(), 10, 8);
    REQUIRE(h.has_value());
    CHECK_EQ(h->first_codepoint, uint32_t{0x20});
    CHECK_EQ(h->line_height, uint32_t{8}); // gemeldete Versalhöhe: Text erscheint 10/8 größer
    REQUIRE(h->glyphs.size() == size_t{'I' - 0x20 + 1});
    CHECK_EQ(h->glyphs['A' - 0x20].w, uint16_t{0}); // fehlt in der Schrift
    CHECK_EQ(h->glyphs[0].advance, uint16_t{4});     // Leerzeichen: 8 × 0,5
}

TEST(font_glyph_metrics_scaled_with_margin) {
    const auto h = cotl::BuildHostFont(MakeFont(), 10, 10);
    REQUIRE(h.has_value());
    const auto& g = h->glyphs['I' - 0x20];
    CHECK_EQ(g.w, uint16_t{2 + 2}); // 4 × 0,5 plus 1 px Rand je Seite
    CHECK_EQ(g.h, uint16_t{10 + 2});
    CHECK_EQ(g.bearing_x, int16_t{0});  // 2 × 0,5 − 1
    CHECK_EQ(g.bearing_y, int16_t{11}); // 20 × 0,5 + 1
    CHECK_EQ(g.advance, uint16_t{5});   // round(4,5)
}

TEST(font_coverage_from_distance_field) {
    const auto h = cotl::BuildHostFont(MakeFont(), 10, 10);
    REQUIRE(h.has_value());
    const auto& g = h->glyphs['I' - 0x20];
    CHECK_EQ(Alpha(*h, g, 1, 5), uint8_t{255}); // im Glyphen
    CHECK_EQ(Alpha(*h, g, 0, 0), uint8_t{0});   // Rand
    CHECK_EQ(h->atlas.rgba[((g.y + 5) * h->atlas.width + g.x + 1) * 4], uint8_t{255}); // RGB weiß
}

TEST(font_without_glyphs_fails) {
    ce_unity::TmpFont f;
    f.cap_line = 20;
    CHECK(!cotl::BuildHostFont(f, 10, 10).has_value());
}

namespace {

// Symbol: voll deckendes 40×40-Quadrat auf durchsichtigem 60×60-Bild
ce_unity::Image IconImage() {
    ce_unity::Image img;
    img.width = img.height = 60;
    img.rgba.assign(60 * 60 * 4, 0);
    for (int y = 10; y < 50; ++y) {
        for (int x = 10; x < 50; ++x) {
            img.rgba[(y * 60 + x) * 4 + 3] = 255;
        }
    }
    return img;
}

} // namespace

TEST(font_icon_glyph_added_as_alpha_mask_scaled_to_height) {
    auto h = cotl::BuildHostFont(MakeFont(), 10, 10);
    REQUIRE(h.has_value());
    const uint32_t old_height = h->atlas.height;
    cotl::AddIconGlyphs(*h, {{0x1E00, IconImage()}}, 10);
    REQUIRE(h->glyphs.size() == size_t{0x1E00 - 0x20 + 1}); // dichter Lauf wächst bis zum Symbol
    const auto& g = h->glyphs[0x1E00 - 0x20];
    CHECK_EQ(g.w, uint16_t{10}); // transparenter Rand abgeschnitten, Seitenverhältnis erhalten
    CHECK_EQ(g.h, uint16_t{10});
    CHECK_EQ(g.bearing_y, int16_t{10}); // steht auf der Grundlinie
    CHECK_EQ(g.advance, uint16_t{11});  // mit 1 px Abstand danach
    CHECK(g.y >= old_height);           // alte Glyphen bleiben unberührt
    CHECK_EQ(h->atlas.height, g.y + uint32_t{10});
    CHECK_EQ(Alpha(*h, g, 5, 5), uint8_t{255});
    CHECK_EQ(h->atlas.rgba[((g.y + 5) * h->atlas.width + g.x + 5) * 4], uint8_t{255}); // RGB weiß
    CHECK_EQ(h->glyphs['I' - 0x20].h, uint16_t{12});
}

TEST(font_icon_empty_image_is_skipped) {
    auto h = cotl::BuildHostFont(MakeFont(), 10, 10);
    REQUIRE(h.has_value());
    ce_unity::Image blank;
    blank.width = blank.height = 4;
    blank.rgba.assign(4 * 4 * 4, 0);
    const size_t before = h->glyphs.size();
    cotl::AddIconGlyphs(*h, {{0x1E00, blank}}, 10);
    CHECK_EQ(h->glyphs.size(), before);
}
