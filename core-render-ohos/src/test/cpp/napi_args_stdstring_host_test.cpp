// Host unit test for leftover getNApiArgsStdString null adopt.
//
// leftover getNApiArgsStdString did std::string(resStr) after
// getNApiArgsString returned nullptr (post napi_throw_error). That is UB.
// adopt_napi_cstr is NAPI-free so this can run on host g++/clang++ without
// a Harmony device or real NAPI.
//
// Build + run (from this directory):
//   ./run_napi_args_stdstring_host_test.sh
//   ./run_napi_args_stdstring_host_test.sh asan

#include "NAPIStringAdopt.h"

#include <cstdio>
#include <cstring>
#include <string>

static int g_failed = 0;

static void expect_eq(const char *name, const std::string &got, const std::string &want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, got.c_str(), want.c_str());
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    // leftover: constructing std::string from nullptr is the bug;
    // helper adopt_napi_cstr(nullptr) returns empty, does not crash.
    expect_eq("adopt_napi_cstr(nullptr)", kuikly::util::adopt_napi_cstr(nullptr), "");

    // leftover: helper on a real C string copies then caller can free.
    char *owned = new char[6];
    std::memcpy(owned, "hello", 6);
    std::string copied = kuikly::util::adopt_napi_cstr(owned);
    expect_eq("adopt_napi_cstr(hello)", copied, "hello");
    delete[] owned;
    expect_eq("adopt copy independent after free", copied, "hello");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
