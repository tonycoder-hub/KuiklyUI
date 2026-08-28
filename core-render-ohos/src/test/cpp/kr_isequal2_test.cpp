// Host unit test for leftover isEqual2 NULL==NULL polarity.
//
// Production isEqual2 treated both-null as unequal. Both-null is now
// equal; xor-null is not; otherwise strcmp. KRCstringEqual.h is
// header-only (no Harmony NDK), so host g++/clang++ is enough.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_isequal2_test.sh
//   ./run_kr_isequal2_test.sh asan

#include "KRCstringEqual.h"

#include <cstdio>

using kuikly::util::isEqual2;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void check_pair(const char *label, const char *a, const char *b, bool want) {
    expect_true(label, isEqual2(a, b) == want);
}

int main() {
    // Required leftover polarity.
    check_pair("isEqual2(nullptr, nullptr) == true", nullptr, nullptr, true);
    check_pair("isEqual2(nullptr, \"a\") == false", nullptr, "a", false);
    check_pair("isEqual2(\"a\", nullptr) == false", "a", nullptr, false);
    check_pair("isEqual2(\"a\", \"a\") == true", "a", "a", true);

    // Extra strcmp / empty cases.
    check_pair("isEqual2(\"a\", \"b\") == false", "a", "b", false);
    check_pair("isEqual2(\"\", \"\") == true", "", "", true);
    check_pair("isEqual2(\"\", nullptr) == false", "", nullptr, false);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
