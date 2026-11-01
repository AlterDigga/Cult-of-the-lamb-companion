// Spine-Atlanten des Spiels (TextAsset "*.atlas", libGDX-Format) für Bilder, die das Spiel nur als
// Spine-Skins zeigt: HUD-Herzen (heart.atlas) und Tarotkarten (WeaponCard.atlas). Siehe
// docs/re-notes.md (Spine-Bilder).
#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "chained_echoes_unity.h"

namespace cotl {

struct SpineRegion {
    int x = 0, y = 0;      // im Atlas, oben links
    int w = 0, h = 0;      // ungedrehte Größe (beschnitten)
    bool rotate = false;   // im Atlas um 90° gedreht abgelegt (belegt h × w)
    int orig_w = 0, orig_h = 0;
    int off_x = 0, off_y = 0; // Lage des Ausschnitts im Originalrahmen (von unten links)
};

struct SpineAtlas {
    std::string page; // erste Seite, z. B. "heart.png"
    int page_w = 0, page_h = 0; // Seitengröße laut Atlas ("size:")
    std::map<std::string, SpineRegion> regions;
};

// Nur die erste Seite (alle benutzten Atlanten haben eine)
SpineAtlas ParseSpineAtlas(std::string_view text);

// Ausschnitt in ungedrehter Lage (w × h · scale); nullopt außerhalb der Seite. scale = Breite der
// Textur / page_w (die Switch-Textur von WeaponCard ist halb so groß wie die Atlasseite).
std::optional<ce_unity::Image> CropRegion(const ce_unity::Image& page, const SpineRegion& r,
                                          float scale = 1.0f);

// Zeichnet src mit Alpha über dst, linke obere Ecke bei (x, y)
void BlitOver(ce_unity::Image& dst, const ce_unity::Image& src, int x, int y);

// Herz wie im HUD (heart.atlas), Originalrahmen zentriert auf kHeartCanvas²: kind red, spirit,
// blue, black, fire, ice; state full, half, empty (empty nur red und spirit). Halbe Herzen sitzen
// links im Rahmen des vollen; ein halbes rotes liegt über dem leeren Behälter.
inline constexpr int kHeartCanvas = 120;
std::optional<ce_unity::Image> ComposeHeart(const SpineAtlas& atlas, const ce_unity::Image& page,
                                            std::string_view kind, std::string_view state);

} // namespace cotl
