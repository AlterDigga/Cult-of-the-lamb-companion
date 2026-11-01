// Lesezugriff auf den Gastspeicher und IL2CPP-Grundtypen (v31).
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct EdenDsmodHostApi;

namespace cotl {

// Abstrakter Gastspeicher: im Modul der Host, in Tests ein FakeGuest.
struct Guest {
    virtual ~Guest() = default;
    virtual bool Read(uint64_t addr, void* out, size_t n) const = 0;
};

// Liest über is_mapped + read_memory des Hosts.
class HostGuest : public Guest {
public:
    explicit HostGuest(const EdenDsmodHostApi* host) : host_(host) {}
    bool Read(uint64_t addr, void* out, size_t n) const override;

private:
    const EdenDsmodHostApi* host_;
};

// IL2CPP v31: Il2CppClass.static_fields, List<T>, Array, String.
inline constexpr uint64_t kClassStaticFields = 0xB8;
inline constexpr uint64_t kListItems = 0x10;
inline constexpr uint64_t kListSize = 0x18;
inline constexpr uint64_t kArrayLength = 0x18;
inline constexpr uint64_t kArrayData = 0x20;
inline constexpr uint64_t kStringLength = 0x10;
inline constexpr uint64_t kStringChars = 0x14;
inline constexpr uint32_t kMaxListSize = 512;

template <class T>
std::optional<T> ReadPod(const Guest& g, uint64_t addr) {
    T v{};
    if (addr == 0 || !g.Read(addr, &v, sizeof(T))) {
        return std::nullopt;
    }
    return v;
}

// Liest einen Zeiger; null zählt als nicht vorhanden.
std::optional<uint64_t> ReadPtr(const Guest& g, uint64_t addr);

// List<T> mit Objektreferenzen. nullopt bei null-Liste, ungültigem _size oder _size > max_length.
std::optional<std::vector<uint64_t>> ReadRefList(const Guest& g, uint64_t list_obj,
                                                 uint32_t max = kMaxListSize);

// List<T> mit Werttypen (int, enum, float).
template <class T>
std::optional<std::vector<T>> ReadValueList(const Guest& g, uint64_t list_obj,
                                            uint32_t max = kMaxListSize);

// System.String → UTF-8. nullopt bei null-Objekt oder Länge außerhalb [0, max_chars].
std::optional<std::string> ReadString(const Guest& g, uint64_t str_obj, uint32_t max_chars = 256);

// static_fields einer Klasse über ihren TypeInfo-Slot in main.
std::optional<uint64_t> StaticFields(const Guest& g, uint64_t main_base, uint64_t typeinfo_off);

} // namespace cotl
