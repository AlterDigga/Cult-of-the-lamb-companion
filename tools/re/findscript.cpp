// Findet in allen Addressables-Bundles das MonoScript mit Klassennamen argv[2] und die
// MonoBehaviours, deren m_Script darauf zeigt (Bundle, PathID, Größe).
// Aufruf: findscript <aa/Switch-Ordner> <Klassenname>
// Bauen: g++ -std=c++20 -O2 -I $COMPANIONS/native/modules -I $EDEN/src findscript.cpp -Lbuild-linux -lce_unity
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <set>
#include "chained_echoes_unity.h"

namespace {
template <class F>
void ForEachFile(const std::string& root, const ce_unity::RangeReader& r, F&& fn) {
    for (const auto& e : std::filesystem::directory_iterator(root)) {
        const std::string name = e.path().filename().string();
        if (!name.ends_with(".bundle")) continue;
        ce_unity::ByteSource s;
        s.size = *r.size(name);
        s.read = [r, name](uint64_t o, uint8_t* out, size_t k) { return r.read(name, o, out, k); };
        auto b = ce_unity::Bundle::Open(s);
        if (!b) continue;
        for (const auto& node : b->Nodes()) {
            if (node.path.ends_with(".resS") || node.path.ends_with(".resource")) continue;
            if (auto f = ce_unity::SerializedFile::Open(b->NodeSource(node))) fn(name, *f);
        }
    }
}
} // namespace

int main(int, char** argv) {
    const std::string root = argv[1];
    const std::string cls = argv[2];
    auto r = ce_unity::MakeDirectoryRangeReader(root);
    std::set<int64_t> scripts;
    ForEachFile(root, r, [&](const std::string& name, const ce_unity::SerializedFile& f) {
        for (const auto& o : f.Objects())
            if (o.class_id == 115 && f.PeekName(o) == cls) {
                std::printf("script %s %lld\n", name.c_str(), (long long)o.path_id);
                scripts.insert(o.path_id);
            }
    });
    std::fflush(stdout);
    if (scripts.empty()) return 1;
    ForEachFile(root, r, [&](const std::string& name, const ce_unity::SerializedFile& f) {
        for (const auto& o : f.Objects()) {
            if (o.class_id != 114 || o.byte_size < 28) continue;
            const auto raw = f.ReadRaw(o);
            if (!raw) continue;
            int64_t pid;
            std::memcpy(&pid, raw->data() + 20, 8);
            if (scripts.count(pid))
                std::printf("behaviour %s %lld size=%u\n", name.c_str(), (long long)o.path_id, o.byte_size);
        }
    });
}
