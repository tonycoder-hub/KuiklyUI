// Host unit test for KRBase64Util leftover Decode polarity.
//
// KRBase64Util.cpp has no OHOS APIs, so this can be built with host g++/clang++.
// Decode was a leftover copy of Encode (re-encodes); it must invert Encode.
//
// Build + run (from this directory):
//   ./run_kr_base64_util_test.sh
//   ./run_kr_base64_util_test.sh asan

#include "KRBase64Util.h"

#include <cstdio>
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
    // Known vector: Encode("hi") is aGk= (must not change Encode).
    expect_eq("Encode(hi)", KRBase64Util::Encode("hi"), "aGk=");

    // Leftover Decode copied Encode and re-encoded aGk= to YUdrPQ==.
    expect_eq("Decode(aGk=)", KRBase64Util::Decode("aGk="), "hi");

    // Round-trip through the public string overloads.
    expect_eq("Decode(Encode(hello))", KRBase64Util::Decode(KRBase64Util::Encode("hello")), "hello");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
