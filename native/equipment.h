// Werte der Ausrüstungskarten wie im Spiel: WeaponInfoCard, CurseInfoCard, RelicInfoCard.
// Formeln: PlayerWeapon.GetDamage und PlayerSpells.GetCurseDamageMultiplier (docs/re-notes.md).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "snapshot.h"

namespace cotl {

inline constexpr int32_t kNoEquipment = 9999; // EquipmentType.None

// Durchschnittsschaden der Waffe, Schaden des Fluchs; nullopt ohne gelesene Daten
std::optional<float> WeaponDamage(const Run& r);
std::optional<float> CurseDamage(const Run& r);
// RelicData.GetChargeCategory (DamageRequiredToCharge)
std::string ChargeCategory(float damage_required);
// IntExtensions.ToNumeral: römisch 1–10, sonst leer
std::string ToNumeral(int32_t n);
// float.ToString mit höchstens zwei Nachkommastellen, ohne Nullen am Ende
std::string FormatStat(float v);

} // namespace cotl
