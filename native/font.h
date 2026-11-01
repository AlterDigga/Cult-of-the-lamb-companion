// Spielschrift für den Companion: TextMesh-Pro-Schrift "LaptureRegular" aus den Spieldateien
// des Spielers (nichts wird mitgeliefert). Die eingebaute Host-Schrift hat nur A–Z ohne Umlaute
// und Apostroph (Thor-Test 5); siehe docs/re-notes.md (Spielschrift).
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "chained_echoes_unity.h"
#include "core/mods/dsmod_module_extensions.h"

namespace cotl {

// Bundle und Name der Schrift (Cult of the Lamb 1.5.27)
inline constexpr const char* kFontBundle = "6685d1be5dc371c74341da1c3dc48c86.bundle";
inline constexpr const char* kFontName = "LaptureRegular";
// Versalhöhe des Atlas in px und die dem Host gemeldete Versalhöhe. Der Host skaliert Glyphen
// nächstliegend auf text_scale × 5 px × (Atlas / gemeldet): 30 / 25 zeichnet die Spielschrift
// 20 % größer als die eingebaute Blockschrift (Kleinbuchstaben wirken sonst kleiner); bei
// text_scale 5 ist das genau 1:1, die Stufen 3–6 bleiben nah daran.
inline constexpr int kFontCapPx = 30;
inline constexpr int kFontLineHeight = 25;

struct HostFont {
    uint32_t line_height = 0; // Host-Konvention: Versalhöhe
    uint32_t first_codepoint = 0;
    std::vector<EdenDsmodFontGlyph> glyphs; // dicht ab first_codepoint, fehlende Zeichen leer
    ce_unity::Image atlas;                  // RGB weiß, Alpha = Deckung
};

// Wandelt das SDF-Atlas der TMP-Schrift in einen Deckungs-Atlas mit Versalhöhe cap_px um
// (bilinear abgetastet, Kante bei d = 0,5, 1 px Rand je Glyph). Zeichen bis U+2026.
// line_height wird dem Host als Versalhöhe gemeldet.
std::optional<HostFont> BuildHostFont(const ce_unity::TmpFont& font, int cap_px, int line_height);

struct IconGlyph {
    uint32_t codepoint = 0;
    ce_unity::Image image; // nur Alpha zählt, der Host färbt Glyphen ein
};

// Hängt Symbole als Glyphen unten an den Atlas: transparenter Rand ab, auf height_px Höhe skaliert,
// auf der Grundlinie stehend. Leere Bilder entfallen.
void AddIconGlyphs(HostFont& font, const std::vector<IconGlyph>& icons, int height_px);

} // namespace cotl
