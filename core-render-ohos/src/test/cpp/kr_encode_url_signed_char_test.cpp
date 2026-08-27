// Host unit test for leftover KREncodeURLComponent signed-char hex-index OOB.
//
// Leftover:
//   for (auto &b : in) { ... HEX_DIGITS_URI[b / 16] ... }
//
// On OHOS aarch64/x86_64 char is signed. UTF-8 high bytes (0xE4 in "你")
// make b/16 == -1 and b%16 == -12. UBSAN: index -1 out of bounds for
// char[17]. Result is garbage ("%") instead of "%E4%BD%A0%E5%A5%BD".
// Sibling KRBase64Encode already iterates unsigned char.
//
// Header-only KRCodecEncode.h so this compiles without Harmony / md5.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_encode_url_signed_char_test.sh
//   ./run_kr_encode_url_signed_char_test.sh asan

#include "KRCodecEncode.h"

#include <cstdio>
#include <string>

using kuikly::KREncodeURLComponent;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq(const char *name, const std::string &got, const std::string &want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, got.c_str(), want.c_str());
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// leftover polarity: signed char 0xE4 indexes HEX_DIGITS_URI[-1] / [-12].
static void leftover_signed_index_polarity() {
    char b = static_cast<char>(0xE4);
    expect_true("host char is signed (leftover precondition)", b < 0);
    expect_true("leftover 0xE4 b/16 is -1 (OOB)", (b / 16) == -1);
    expect_true("leftover 0xE4 b%16 is -12 (OOB)", (b % 16) == -12);

    unsigned char ub = static_cast<unsigned char>(0xE4);
    expect_true("leftover unsigned 0xE4 b/16 is 14", (ub / 16) == 14);
    expect_true("leftover unsigned 0xE4 b%16 is 4", (ub % 16) == 4);
}

int main() {
    leftover_signed_index_polarity();

    // leftover: ASCII unreserved stays unescaped.
    expect_eq("hello", KREncodeURLComponent("hello"), "hello");
    expect_eq("unreserved", KREncodeURLComponent("AZaz09-_.!~*'()"), "AZaz09-_.!~*'()");

    // leftover: reserved / non-unreserved ASCII is percent-encoded (uppercase).
    expect_eq("space", KREncodeURLComponent(" "), "%20");
    expect_eq("hello world", KREncodeURLComponent("hello world"), "hello%20world");
    expect_eq("slash", KREncodeURLComponent("a/b"), "a%2Fb");

    // leftover: UTF-8 "你好" (E4 BD A0 E5 A5 BD) used to OOB on signed char.
    expect_eq("nihao", KREncodeURLComponent("你好"), "%E4%BD%A0%E5%A5%BD");

    expect_eq("empty", KREncodeURLComponent(""), "");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover encode tests passed\n");
    return 0;
}
