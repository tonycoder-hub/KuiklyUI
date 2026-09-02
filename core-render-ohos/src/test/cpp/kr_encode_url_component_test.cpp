// Host unit test for KREncodeURLComponent.
//
// KRCodec.cpp has no OHOS APIs, so this can be built with host g++/clang++.
// Covers signed-char OOB on UTF-8 high bytes (OHOS aarch64/x86_64 char is signed).
//
// Build + run (from this directory):
//   ./run_kr_codec_test.sh
//   ./run_kr_codec_test.sh asan

#include "KRCodec.h"

#include <cstdio>
#include <string>

using kuikly::KREncodeURLComponent;

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
    // ASCII unreserved characters stay unescaped.
    expect_eq("hello", KREncodeURLComponent("hello"), "hello");
    expect_eq("unreserved", KREncodeURLComponent("AZaz09-_.!~*'()"), "AZaz09-_.!~*'()");

    // Reserved / non-unreserved ASCII is percent-encoded (uppercase hex).
    expect_eq("space", KREncodeURLComponent(" "), "%20");
    expect_eq("slash", KREncodeURLComponent("a/b"), "a%2Fb");

    // UTF-8 Chinese: bytes E4 BD A0 E5 A5 BD must not OOB on signed char.
    expect_eq("nihao", KREncodeURLComponent("你好"), "%E4%BD%A0%E5%A5%BD");

    // Empty input is a no-op.
    expect_eq("empty", KREncodeURLComponent(""), "");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
