// Englische Texte aus der I2-Localization des Spiels (MonoBehaviour "I2Languages" in
// romfs:/Data/resources.assets). Es wird nichts mitgeliefert, alles stammt aus den Spieldateien.
#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "chained_echoes_unity.h"

namespace cotl {

class I2Table {
public:
    // Liest die englische Spalte aus `path` (Standard Data/resources.assets).
    static std::optional<I2Table> Load(const ce_unity::RangeReader& romfs,
                                       std::string_view path = "Data/resources.assets");
    // Für Tests: fertige Begriff → Text-Zuordnung.
    static I2Table FromMap(std::map<std::string, std::string> terms);

    std::optional<std::string> English(std::string_view term) const;
    size_t Size() const { return terms_.size(); }

private:
    std::map<std::string, std::string, std::less<>> terms_;
};

} // namespace cotl
