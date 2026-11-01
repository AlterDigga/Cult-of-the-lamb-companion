#include "test_util.h"

int main() {
    for (const auto& c : test::Registry()) {
        const int before = test::Failures();
        c.fn();
        std::printf("%s %s\n", test::Failures() == before ? "ok  " : "FAIL", c.name);
    }
    std::printf("%zu tests, %d failed checks\n", test::Registry().size(), test::Failures());
    return test::Failures() == 0 ? 0 : 1;
}
