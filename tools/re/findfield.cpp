// Sucht in allen Addressables-Bundles MonoBehaviours mit einem bestimmten Feld (TypeTree) und gibt
// Bundle und PathID aus. Aufruf: findfield <aa/Switch-Ordner> <Feldname>
// Bauen: g++ -std=c++20 -O2 -I $COMPANIONS/native/modules -I $EDEN/src findfield.cpp -Lbuild-linux -lce_unity
#include <cstdio>
#include <filesystem>
#include "chained_echoes_unity.h"

int main(int, char** argv) {
    const std::string root = argv[1];
    const std::string field = argv[2];
    auto r = ce_unity::MakeDirectoryRangeReader(root);
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
            auto f = ce_unity::SerializedFile::Open(b->NodeSource(node));
            if (!f || !f->HasTypeTrees()) continue;
            for (const auto& o : f->Objects()) {
                if (o.class_id != 114) continue;
                const auto v = f->ReadObject(o);
                if (v && v->Get(field)) {
                    std::printf("%s %lld\n", name.c_str(), (long long)o.path_id);
                    std::fflush(stdout);
                }
            }
        }
    }
}
