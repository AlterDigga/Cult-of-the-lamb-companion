// ABI-Einstieg des Companion-Moduls (MODULE_GUIDE.md §1).
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "core/mods/dsmod_module_abi.h"
#include "core/mods/dsmod_module_extensions.h"
#include "core/mods/modules/dsmod_module_sdk.h"
#include "font.h"
#include "guest.h"
#include "i2.h"
#include "icons.h"
#include "offsets_1_5_27.h"
#include "problems.h"
#include "publish.h"
#include "quests.h"
#include "snapshot.h"

namespace {

// Listen alle 10 Ticks neu lesen (6 Hz); dazwischen den letzten Stand erneut veröffentlichen,
// weil der Host die Werte jeden Tick löscht.
constexpr uint32_t kReadEvery = 10;

// Solange nach einem Seitenwechsel alle Werte gehen (Überblendung 120 ms, Sicherheit)
constexpr int kAllPagesTicks = 30;

struct Selections {
    int64_t follower = -1, problem_filter = -1, problem = -1, quest = -1, item = -1, tarot = -1;
    bool texts = false;
    bool operator==(const Selections&) const = default;
};

struct Instance {
    EdenDsmodHostApi host; // Kopie: Zeiger in Callbacks sind nur geliehen
    std::optional<cotl::GameSnapshot> last;
    uint32_t ticks = 0;
    // I2-Texte: im Hintergrund geladen (12 MB parsen würde einen Tick blockieren)
    std::unique_ptr<cotl::I2Table> i2;
    std::atomic<bool> i2_ready{false};
    std::thread i2_loader;
    // Icons: nur im Asset-Worker benutzt (load_image), getrennt vom Tick-Thread
    std::mutex icons_mutex;
    EdenDsmodHostExtensions ext{};
    std::unique_ptr<cotl::IconLoader> icons;
    // Spielschrift: einmal geladen, von decode_font (Host-Thread) und load_image (Asset-Worker)
    std::mutex font_mutex;
    std::optional<cotl::HostFont> font;
    cotl::ProblemSelection problem_selection;
    cotl::KeepSelection<int32_t> follower_selection;
    cotl::KeepSelection<std::string> quest_selection;
    cotl::KeepSelection<int32_t> item_selection;
    // Tarot-Fenster: vom Modul gehalten, damit es mit dem Run schließt (Flags kann nur das Paket setzen)
    std::atomic<int64_t> run_open{-1};
    // Fertige Werte; wiedergegeben jeden Tick, neu aufgebaut nur bei neuem Stand oder neuer Auswahl
    cotl::RecordingPublisher values;
    Selections selections;
    bool built = false;
    std::string page;
    int all_pages_ticks = 0;
};

// icons_mutex gehalten
void EnsureIcons(Instance* inst) {
    if (!inst->icons) {
        inst->icons = std::make_unique<cotl::IconLoader>(
            ce_unity::MakeHostRangeReader(&inst->host),
            cotl::MakeTiledAstcDecoder(inst->ext.userdata, inst->ext.decode_astc));
    }
}

// Lädt die Spielschrift beim ersten Bedarf; false, solange das romfs nicht lesbar ist
bool EnsureFont(Instance* inst) {
    std::lock_guard lock(inst->font_mutex);
    if (inst->font) {
        return true;
    }
    if (inst->host.read_romfs == nullptr) {
        return false;
    }
    ce_unity::Reader reader(ce_unity::MakeHostRangeReader(&inst->host),
                            cotl::MakeTiledAstcDecoder(inst->ext.userdata, inst->ext.decode_astc));
    const auto tmp = ce_unity::LoadTmpFont(reader, cotl::kFontBundle, cotl::kFontName, true);
    if (tmp) {
        inst->font = cotl::BuildHostFont(*tmp, cotl::kFontCapPx, cotl::kFontLineHeight);
    }
    if (!inst->font) {
        return false;
    }
    // Herz-Symbole für Texte (cotl::TextWithIcons); fehlen sie, bleibt die Stelle im Text leer
    std::lock_guard icons_lock(inst->icons_mutex);
    EnsureIcons(inst);
    std::vector<cotl::IconGlyph> glyphs;
    const auto add = [&](uint32_t cp, const char* kind, const char* state) {
        if (auto img = inst->icons->LoadHeart(kind, state)) {
            glyphs.push_back({cp, std::move(*img)});
        }
    };
    add(cotl::kIconHeart, "red", "full");
    add(cotl::kIconHalfHeart, "blue", "half"); // ohne Behälter: nur die halbe Form
    add(cotl::kIconIceHeart, "ice", "full");
    add(cotl::kIconFireHeart, "fire", "full");
    cotl::AddIconGlyphs(*inst->font, glyphs, cotl::kFontCapPx);
    return true;
}

void LoadTexts(Instance* inst) {
    try {
        if (auto t = cotl::I2Table::Load(ce_unity::MakeHostRangeReader(&inst->host))) {
            inst->i2 = std::make_unique<cotl::I2Table>(std::move(*t));
            inst->i2_ready.store(true, std::memory_order_release);
        }
    } catch (...) {
    }
}

EdenDsmodBool SupportsBuild(const char* build_hex) {
    return build_hex != nullptr && std::strcmp(build_hex, cotl::off::kBuildIdHex) == 0
               ? EDEN_DSMOD_TRUE
               : EDEN_DSMOD_FALSE;
}

void* Create(const EdenDsmodHostApi* host, const char*) {
    try {
        if (!dsmod_sdk::HostAbiMatches(host) || host->title_id != cotl::off::kTitleId) {
            return nullptr;
        }
        uint8_t expected[0x20]{};
        std::memcpy(expected, cotl::off::kBuildId, sizeof(cotl::off::kBuildId));
        if (std::memcmp(host->build_id, expected, sizeof(expected)) != 0) {
            return nullptr;
        }
        auto* inst = new (std::nothrow) Instance{};
        if (inst != nullptr) {
            inst->host = *host;
            if (host->read_romfs != nullptr) {
                inst->i2_loader = std::thread(LoadTexts, inst);
            }
        }
        return inst;
    } catch (...) {
        return nullptr;
    }
}

void Destroy(void* instance) {
    auto* inst = static_cast<Instance*>(instance);
    if (inst != nullptr && inst->i2_loader.joinable()) {
        inst->i2_loader.join();
    }
    delete inst;
}

// Alle Werte aus dem letzten Lesestand neu aufbauen; sie werden dann jeden Tick weitergegeben
void Rebuild(Instance* inst, const Selections& now) {
    auto& rec = inst->values;
    rec.Clear();
    if (!inst->last) {
        rec.I64("g.in_run", 0); // Tab-Wechsel braucht den Wert immer
        rec.I64("g.loaded", 0);
        return;
    }
    const auto& s = *inst->last;
    const cotl::Terms terms(now.texts ? inst->i2.get() : nullptr);
    cotl::PublishFollowers(rec, s, terms);
    const auto& follower = inst->follower_selection.Current();
    cotl::PublishSelectedFollower(rec, s, terms, follower ? cotl::FollowerRowOf(s, *follower) : -1);
    const auto problems = cotl::DeriveProblems(s, terms, static_cast<int32_t>(now.problem_filter));
    cotl::PublishProblems(rec, s, problems);
    cotl::PublishSelectedProblems(rec, s, inst->problem_selection.Update(now.problem, problems));
    const auto quests = cotl::BuildQuests(s, terms);
    const auto& quest = inst->quest_selection.Update(now.quest, [&](int64_t row) {
        return row < static_cast<int64_t>(quests.size()) ? quests[static_cast<size_t>(row)].key : std::string{};
    });
    int64_t quest_row = -1;
    for (size_t i = 0; quest && i < quests.size(); ++i) {
        quest_row = quests[i].key == *quest ? static_cast<int64_t>(i) : quest_row;
    }
    cotl::PublishQuests(rec, quests, quest_row);
    const auto& item = inst->item_selection.Update(now.item, [&](int64_t selected) {
        return cotl::InventoryTypeAt(s, selected).value_or(-1);
    });
    cotl::PublishInventory(rec, s, terms, item ? cotl::InventoryRowOf(s, *item) : -1);
    cotl::PublishRun(rec, s, terms, now.tarot);
    cotl::PublishMap(rec, s);
    cotl::PublishCult(rec, s);
}

void Sample(void* instance, const EdenDsmodHostApi* host) {
    try {
        if (instance == nullptr || host == nullptr || host->publish_i64 == nullptr) {
            return;
        }
        auto* inst = static_cast<Instance*>(instance);
        host->publish_i64(host->userdata, "mod.ready", 1);
        const auto flag = [&](const char* name) {
            return host->get_i64 ? host->get_i64(host->userdata, name, -1) : -1;
        };
        const Selections now{flag("@flag:fol.open"),   flag("@flag:prob.filter"),
                             flag("@flag:prob.open"),  flag("@flag:quest.open"),
                             flag("@flag:inv.open"),   inst->run_open.load(std::memory_order_relaxed),
                             inst->i2_ready.load(std::memory_order_acquire)};
        // Ein neu angetippter Follower: Gedanken sofort lesen, sonst leeres Fenster bis zum Lesetakt
        const auto before = inst->follower_selection.Current();
        const auto& follower = inst->follower_selection.Update(now.follower, [&](int64_t row) {
            return inst->last ? cotl::FollowerIdAtRow(*inst->last, row).value_or(-1) : -1;
        });
        bool dirty = now != inst->selections;
        if (inst->ticks++ % kReadEvery == 0 || follower != before) {
            const cotl::HostGuest guest(host);
            const auto statics =
                cotl::StaticFields(guest, host->main_base, cotl::off::kDataManagerTypeInfo);
            const bool has_game =
                statics && cotl::ReadPtr(guest, *statics + cotl::off::kDataManagerInstance);
            if (!has_game) {
                dirty = dirty || inst->last.has_value();
                inst->last.reset(); // vor dem Titelbild oder beim Laden
            } else if (auto snap = cotl::ReadSnapshot(guest, host->main_base,
                                                      follower.value_or(cotl::kNoThoughts),
                                                      inst->run_open.load(std::memory_order_relaxed) >=
                                                          cotl::kRunSelWeapon)) {
                inst->last = std::move(snap);
                dirty = true;
            }
            // Bei einem Torn-Read bleibt der letzte gültige Stand stehen
        }
        // Tarot-Fenster gehört zum Run: danach geschlossen, auch wenn es beim Tod offen war
        if (!(inst->last && inst->last->run) && inst->run_open.exchange(-1) != -1) {
            dirty = true;
        }
        if (dirty || !inst->built) {
            inst->selections = now;
            inst->built = true;
            Rebuild(inst, now);
        }
        // Während eines Seitenwechsels (Überblendung zeigt beide Seiten) alle Werte weitergeben
        const char* page = host->get_text ? host->get_text(host->userdata, "__page") : nullptr;
        if (page != nullptr && inst->page != page) {
            inst->page = page;
            inst->all_pages_ticks = kAllPagesTicks;
        }
        std::optional<cotl::PageGroup> group;
        if (inst->all_pages_ticks > 0) {
            --inst->all_pages_ticks;
        } else if (page != nullptr) {
            group = cotl::GroupOfPage(inst->page);
        }
        cotl::HostPublisher pub(host);
        inst->values.Replay(pub, group);
    } catch (...) {
    }
}

void Tick(void*, const EdenDsmodHostApi*) {}

void Configure(void* instance, const EdenDsmodHostExtensions* ext) {
    if (instance != nullptr && ext != nullptr) {
        static_cast<Instance*>(instance)->ext = *ext; // Kopie: Zeiger ist nur geliehen
    }
}

// Nur Anzeige-Zustand des Companions, nichts wirkt auf das Spiel
EdenDsmodBool OnAction(void* instance, const char* action, int64_t argument) {
    auto* inst = static_cast<Instance*>(instance);
    if (inst == nullptr || action == nullptr) {
        return EDEN_DSMOD_FALSE;
    }
    if (std::strcmp(action, "run.select") == 0) {
        inst->run_open.store(argument, std::memory_order_relaxed);
        return EDEN_DSMOD_TRUE;
    }
    if (std::strcmp(action, "run.close") == 0) {
        inst->run_open.store(-1, std::memory_order_relaxed);
        return EDEN_DSMOD_TRUE;
    }
    return EDEN_DSMOD_FALSE;
}

// Bilder aus den Spieldateien: module:cotl:item/<ITEM_TYPE> (Inventar), equip/<EquipmentType>,
// relic/<RelicType>, tarot/<Card>, heart/<art>_<zustand>, mapicon/<Sprite> (native/icons.h); module:cotl:font →
// Atlas der Spielschrift
EdenDsmodBool LoadImage(void* instance, const EdenDsmodHostApi*, const char* key, void* receiver,
                        EdenDsmodImageSink sink) {
    try {
        constexpr std::string_view kPrefix = "module:cotl:";
        auto* inst = static_cast<Instance*>(instance);
        const std::string_view k = key != nullptr ? key : "";
        if (inst != nullptr && k == "module:cotl:font" && sink != nullptr) {
            if (!EnsureFont(inst)) {
                return EDEN_DSMOD_FALSE;
            }
            const auto& a = inst->font->atlas;
            sink(receiver, a.width, a.height, a.rgba.data(), a.rgba.size());
            return EDEN_DSMOD_TRUE;
        }
        if (inst == nullptr || !k.starts_with(kPrefix) || inst->host.read_romfs == nullptr) {
            return EDEN_DSMOD_FALSE;
        }
        const std::string_view rest = k.substr(kPrefix.size()); // "<art>/<schlüssel>"
        const auto slash = rest.find('/');
        if (slash == std::string_view::npos) {
            return EDEN_DSMOD_FALSE;
        }
        const std::string_view kind = rest.substr(0, slash);
        const std::string arg(rest.substr(slash + 1));
        const auto num = static_cast<int32_t>(std::strtol(arg.c_str(), nullptr, 10));
        std::lock_guard lock(inst->icons_mutex);
        EnsureIcons(inst);
        std::optional<ce_unity::Image> img;
        if (kind == "item") {
            img = inst->icons->LoadItem(num);
        } else if (kind == "equip") {
            img = inst->icons->LoadEquipment(num);
        } else if (kind == "relic") {
            img = inst->icons->LoadRelic(num);
        } else if (kind == "mapicon") {
            img = inst->icons->LoadMapIcon(arg);
        } else if (kind == "tarot") {
            img = inst->icons->LoadTarot(num);
        } else if (kind == "heart") { // "<art>_<zustand>"
            const auto us = arg.find('_');
            if (us != std::string::npos) {
                img = inst->icons->LoadHeart(arg.substr(0, us), arg.substr(us + 1));
            }
        }
        if (!img || sink == nullptr) {
            if (!img && inst->host.log != nullptr) {
                const std::string msg = "cotl icon " + std::string(k) + ": " + inst->icons->LastError();
                inst->host.log(inst->host.userdata, EDEN_DSMOD_LOG_WARNING, msg.c_str());
            }
            return EDEN_DSMOD_FALSE;
        }
        sink(receiver, img->width, img->height, img->rgba.data(), img->rgba.size());
        return EDEN_DSMOD_TRUE;
    } catch (...) {
        return EDEN_DSMOD_FALSE;
    }
}

// Manifest-"font" ist die Paketdatei cotl_font.txt ("COTL_FONT_V1 …"); die Metriken kommen aus
// der Spielschrift (native/font.h), der Atlas über module:cotl:font.
EdenDsmodBool DecodeFont(void* instance, const uint8_t* bytes, size_t size, void* receiver,
                         EdenDsmodFontSink sink) {
    try {
        constexpr std::string_view kMagic = "COTL_FONT_V1";
        auto* inst = static_cast<Instance*>(instance);
        if (inst == nullptr || sink == nullptr || bytes == nullptr ||
            std::string_view(reinterpret_cast<const char*>(bytes), size).substr(0, kMagic.size()) != kMagic ||
            !EnsureFont(inst)) {
            return EDEN_DSMOD_FALSE; // der Host versucht es später erneut
        }
        const auto& f = *inst->font;
        sink(receiver, f.line_height, f.first_codepoint, f.glyphs.data(),
             static_cast<uint32_t>(f.glyphs.size()));
        return EDEN_DSMOD_TRUE;
    } catch (...) {
        return EDEN_DSMOD_FALSE;
    }
}

const EdenDsmodFontExtensions kFontExtensions = {
    EDEN_DSMOD_FONT_EXT_VERSION, sizeof(EdenDsmodFontExtensions), EDEN_DSMOD_FONT_EXT_HASH, DecodeFont,
};

const EdenDsmodModuleExtensions kExtensions = {
    EDEN_DSMOD_EXT_VERSION, sizeof(EdenDsmodModuleExtensions), EDEN_DSMOD_EXT_HASH,
    Configure, OnAction, LoadImage,
};

const EdenDsmodModuleApi kModule = {
    EDEN_DSMOD_MODULE_ABI_VERSION,
    sizeof(EdenDsmodModuleApi),
    0,
    EDEN_DSMOD_MODULE_ABI_HASH,
    cotl::off::kTitleId,
    "Cult of the Lamb companion",
    EDEN_DSMOD_CAP_NO_TICK_WHEN_HIDDEN | EDEN_DSMOD_CAP_EXTENSIONS,
    SupportsBuild,
    Create,
    Destroy,
    Sample,
    Tick,
};

} // namespace

DSMOD_SDK_EXPORT_MODULE(kModule)
DSMOD_SDK_EXPORT_EXTENSIONS(kExtensions)
DSMOD_SDK_EXPORT_FONT_EXTENSIONS(kFontExtensions)
