// Enum-Wert → Spieltext. Begriffsformate aus dem Spielcode, siehe docs/re-notes.md (I2-Begriffe).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cotl {

class I2Table;

// Entfernt Rich-Text-Markup (<color=…>, <sprite …>, </…>), das die Companion-Schrift nicht kann.
std::string StripMarkup(std::string_view s);

// Symbole im Text: zusätzliche Zeichen der Companion-Schrift (native/font.h) aus den HUD-Herzen.
// Unbenutzter Bereich unterhalb der höchsten Schriftzeichen (U+2026), damit der dichte Lauf reicht.
inline constexpr uint32_t kIconHeart = 0x1E00;
inline constexpr uint32_t kIconHalfHeart = 0x1E01;
inline constexpr uint32_t kIconIceHeart = 0x1E02;
inline constexpr uint32_t kIconFireHeart = 0x1E03;
inline constexpr const char* kIconColorRed = "#FFE0352B";
inline constexpr const char* kIconColorBlue = "#FF19B8A6";
inline constexpr const char* kIconColorBlack = "#FF8C8C9C";
inline constexpr const char* kIconColorIce = "#FF7FD0F0";
inline constexpr const char* kIconColorFire = "#FFF0A030";

// Spieltext für Labels mit color_markup: <sprite name="icon_…Heart"> als farbiges Symbol,
// <color=#RRGGBB> als Farb-Span, <br> als Zeilenumbruch; andere Tags und Sprites entfallen.
std::string TextWithIcons(std::string_view s);

class Terms {
public:
    explicit Terms(const I2Table* table) : table_(table) {}

    std::string Trait(int32_t v) const;      // Traits/{TraitType}
    std::string Role(int32_t v) const;       // Traits/{FollowerRole}
    std::string Item(int32_t v) const;       // Inventory/{ITEM_TYPE}
    // Inventory/{ITEM_TYPE}/Description ohne Markup; leer ohne Tabelle oder Begriff
    std::string ItemDescription(int32_t v) const;
    std::string TarotName(int32_t v) const;  // TarotCards/{Card}/Name
    std::string TarotText(int32_t v) const;  // TarotCards/{Card}/Description
    std::string Weapon(int32_t v) const;     // UpgradeSystem/{EquipmentType}/Name
    std::string Curse(int32_t v) const;      // UpgradeSystem/{EquipmentType}/Name
    std::string Relic(int32_t v) const;      // Relics/{RelicType}
    std::string EquipmentText(int32_t v) const; // UpgradeSystem/{EquipmentType}/Description
    std::string EquipmentLore(int32_t v) const; // UpgradeSystem/{EquipmentType}/Lore
    std::string RelicText(int32_t v) const;     // Relics/{RelicType}/Description
    std::string RelicLore(int32_t v) const;     // Relics/{RelicType}/Lore
    std::string Structure(int32_t v) const;  // Structures/{StructureBrain.TYPES}
    std::string Meal(int32_t v) const;       // CookingData/{ITEM_TYPE}/Name
    std::string Thought(int32_t v) const;    // FollowerThoughts/{Thought}

    // Englischer Text eines beliebigen Begriffs; nullopt ohne Tabelle oder Begriff
    std::optional<std::string> Raw(std::string_view term) const;

    // Code-Bezeichner eines Enum-Werts ("TraitType", 3 → "DesensitisedToDeath");
    // unbekannter Wert → "#<wert>".
    static std::string EnumName(std::string_view enum_name, int32_t v);

private:
    // Beschreibung oder Lore; Abwandlungen ohne eigenen Text nehmen den der Grundform, sonst leer
    std::string GearText(std::string_view enum_name, int32_t v, std::string_view prefix,
                         std::string_view suffix) const;
    // Text zu `prefix + Bezeichner + suffix`; ohne Tabelle oder Begriff der Bezeichner selbst.
    std::string Lookup(std::string_view enum_name, int32_t v, std::string_view prefix,
                       std::string_view suffix) const;

    const I2Table* table_;
};

} // namespace cotl
