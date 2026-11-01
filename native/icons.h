// Gegenstands-Icons aus den Spieldateien des Spielers (nichts wird mitgeliefert).
//
// Weg: "Inventory Icon Mapping" (MonoBehaviour in resources.assets: ITEM_TYPE → Sprite) →
// Sprite (RenderDataKey + Atlas-Tag, Rohlayout Unity 2022.3) → SpriteAtlas im Addressables-Bundle
// (atlas_bundles_1_5_27.inc, mit TypeTree) → Rechteck in der Atlas-Textur → ausschneiden.
// Details in docs/re-notes.md (Inventar-Icons).
//
// Ebenso Waffen/Flüche (WeaponData/CurseData → UISprite) und Reliquien (RelicData → UISprite) aus
// resources.assets; HUD-Herzen und Tarotkarten aus Spine-Atlanten (spine_atlas.h).
#pragma once

#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include "chained_echoes_unity.h"

namespace cotl {

// "Inventory Icon Mapping" roh: ITEM_TYPE → Sprite-PathID (nur Verweise in dieselbe Datei)
std::optional<std::map<int32_t, int64_t>> ParseIconMapping(const std::vector<uint8_t>& raw);

struct SpriteRef {
    std::array<uint32_t, 4> guid{}; // m_RenderDataKey.first
    int64_t second = 0;             // m_RenderDataKey.second
    std::string atlas_tag;          // erster m_AtlasTags-Eintrag, leer ohne Atlas
    float x = 0, y = 0, w = 0, h = 0; // m_Rect
    int64_t texture = 0;            // m_RD.texture: eigene Textur in derselben Datei, 0 im Atlas
};

// m_Script-PathIDs in globalgamemanagers.assets (1.5.27)
inline constexpr int64_t kScriptCurseData = 372;
inline constexpr int64_t kScriptEquipmentData = 1116;
inline constexpr int64_t kScriptRelicData = 2731;
inline constexpr int64_t kScriptWeaponData = 3987;

enum class DataKind { Equipment, Relic };

struct DataSprite {
    DataKind kind = DataKind::Equipment;
    int32_t type = 0;   // EquipmentType bzw. RelicType
    int64_t sprite = 0; // UISprite-PathID in derselben Datei
};

// EquipmentData-/RelicData-MonoBehaviour roh bis UISprite; nullopt bei anderen Skripten
std::optional<DataSprite> ParseDataSprite(const std::vector<uint8_t>& raw);

// Sprite-Objekt roh (Unity 2022.3, ohne TypeTree) bis einschließlich m_RD.texture
std::optional<SpriteRef> ParseSpriteRef(const std::vector<uint8_t>& raw);

// Texture2D-Objekt roh (Unity 2022.3, ohne TypeTree); Bilddaten im m_StreamData
std::optional<ce_unity::TextureInfo> ParseRawTexture(const std::vector<uint8_t>& raw);

// Schneidet ein Rechteck in Unity-Koordinaten (Ursprung unten links) aus einem Bild mit
// Ursprung oben links aus.
std::optional<ce_unity::Image> CropUnityRect(const ce_unity::Image& tex, float x, float y, float w,
                                              float h);

// Ausschnitt einer ASTC-12×12-Textur im Tegra-Blocklayout (m_PlatformBlob[8]: log2 GOBs je Block);
// Rechteck in Unity-Koordinaten, Bild mit Ursprung oben links.
std::optional<ce_unity::Image> DecodeSwitchSprite(std::span<const uint8_t> data,
                                                  const ce_unity::TextureInfo& tex,
                                                  const ce_unity::AstcDecoder& astc, float x, float y,
                                                  float w, float h);

// ASTC-Decoder über den Host, der große Texturen in Kacheln aus ganzen Blöcken dekodiert:
// ce_unity::MakeHostAstcDecoder lehnt Ausgaben > 16 MiB ab, Edens decode_astc Breiten/Höhen
// > 2048 (2048er-Atlanten sind auf 2052 gepolstert). max_tile_bytes begrenzt die Ausgabe je
// Host-Aufruf, max_width die Kachelbreite (und -höhe).
ce_unity::AstcDecoder MakeTiledAstcDecoder(void* userdata, EdenDsmodAstcDecoder decode,
                                           size_t max_tile_bytes = 8u << 20,
                                           uint32_t max_width = 2048);

struct IconLocation {
    std::string bundle;   // Addressables-Bundle relativ zu Data/StreamingAssets/aa/Switch/
    std::string texture;  // Name der Atlas-Textur
    float x = 0, y = 0, w = 0, h = 0;
    uint32_t settings = 0; // settingsRaw (Bit 0 gepackt, Bits 2–5 Drehung)
};

class IconLoader {
public:
    IconLoader(ce_unity::RangeReader romfs, ce_unity::AstcDecoder astc);
    ~IconLoader();

    // Ort des Icons ohne Dekodieren (offline testbar); nullopt ohne Atlas-Eintrag
    std::optional<IconLocation> Locate(int32_t item_type);
    // RGBA-Icon; dekodierte Atlas-Texturen werden zwischengespeichert
    std::optional<ce_unity::Image> LoadItem(int32_t item_type);
    // Waffe oder Fluch (EquipmentType), Reliquie (RelicType)
    std::optional<ce_unity::Image> LoadEquipment(int32_t type);
    std::optional<ce_unity::Image> LoadRelic(int32_t type);
    // Herz wie im HUD: kind red/spirit/blue/black/fire/ice, state full/half/empty
    std::optional<ce_unity::Image> LoadHeart(const std::string& kind, const std::string& state);
    // Vorderseite einer Tarotkarte (TarotCards.Card)
    std::optional<ce_unity::Image> LoadTarot(int32_t card);
    // Sprite der Spielkarte aus dem MapIcons-Atlas (Raumsymbole "Icon_Boss" …, "Map_You")
    std::optional<ce_unity::Image> LoadMapIcon(const std::string& sprite);
    // Grund des letzten Fehlschlags (für das Log)
    std::string LastError() {
        std::lock_guard lock(mutex_);
        return last_error_;
    }

private:
    bool EnsureMapping();
    bool EnsureData();
    std::optional<IconLocation> LocateSprite(int64_t sprite_path_id); // mutex_ gehalten
    std::optional<IconLocation> LocateRef(const SpriteRef& ref);       // mutex_ gehalten
    std::optional<ce_unity::Image> LoadLocated(const std::optional<IconLocation>& loc,
                                               const std::string& what);
    // Icon mit eigener Textur in resources.assets (ohne TypeTree, Bilddaten in .resS)
    std::optional<ce_unity::Image> LoadOwnTexture(int32_t item_type);
    // Spine-Atlas "<name>_Atlas" im Spine-Bundle samt dekodierter Seite (mutex_ gehalten)
    struct SpinePage;
    const SpinePage* EnsureSpine(const std::string& atlas_asset);

    ce_unity::RangeReader romfs_;
    ce_unity::AstcDecoder astc_;
    std::unique_ptr<ce_unity::Reader> bundles_;
    std::shared_ptr<ce_unity::SerializedFile> resources_;
    std::optional<std::map<int32_t, int64_t>> mapping_;
    std::optional<std::map<int32_t, int64_t>> equipment_, relics_; // Typ → Sprite-PathID
    std::map<std::string, std::unique_ptr<SpinePage>> spine_;
    using AtlasKey = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, int64_t>;
    std::map<std::string, std::map<AtlasKey, IconLocation>> atlases_; // Atlas-Tag → seine Sprites
    std::list<std::pair<std::string, ce_unity::Image>> textures_;     // zuletzt benutzte zuerst
    std::string last_error_;
    std::mutex mutex_;
};

} // namespace cotl
