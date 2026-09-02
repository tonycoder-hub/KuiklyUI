// Host unit test for leftover KRDecodeURLComponent signed-char ctype.
//
// leftover KRDecodeURLComponent passed raw char to isxdigit/isdigit/toupper.
// Those require unsigned char or EOF; on OHOS aarch64/x86_64 char is signed,
// so high bytes like "%\x80\x80" were UB, not a clean reject.
//
// KRCodecDecode.h is Harmony-free (and md5/sha256-free) so this builds with
// host g++/clang++. Forced -fsigned-char matches OHOS.
//
// Build + run (from this directory):
//   ./run_kr_decode_url_test.sh
//   ./run_kr_decode_url_test.sh asan

#include "KRCodecDecode.h"

#include <climits>
#include <cstdio>
#include <string>

static_assert(CHAR_MIN < 0, "test requires signed char (use -fsigned-char)");

using kuikly::KRDecodeHexNibble;
using kuikly::KRDecodeURLComponent;

static int g_failed = 0;

static void dump_bytes(FILE *fp, const std::string &s) {
    for (unsigned char c : s) {
        std::fprintf(fp, "%02X", c);
    }
}

static void expect_eq(const char *name, const std::string &got, const std::string &want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got ", name);
        dump_bytes(stderr, got);
        std::fprintf(stderr, " want ");
        dump_bytes(stderr, want);
        std::fprintf(stderr, "\n");
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_int(const char *name, int got, int want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    // leftover: raw char 0x80 fails the isxdigit contract on signed-char hosts.
    // Helper takes unsigned char, so 0x80 is a clean non-hex reject, not UB.
    expect_int("nibble high byte", KRDecodeHexNibble(0x80), -1);
    expect_int("nibble A", KRDecodeHexNibble(static_cast<unsigned char>('A')), 10);
    expect_int("nibble f", KRDecodeHexNibble(static_cast<unsigned char>('f')), 15);

    // leftover: "%\x80\x80" must not UB; invalid %xx is kept as literal '%'.
    {
        std::string in;
        in.push_back('%');
        in.push_back(static_cast<char>(0x80));
        in.push_back(static_cast<char>(0x80));
        expect_eq("high-byte percent", KRDecodeURLComponent(in), in);
    }

    // Valid percent-decoding, including a high decoded byte.
    expect_eq("%41", KRDecodeURLComponent("%41"), "A");
    expect_eq("%4f", KRDecodeURLComponent("%4f"), "O");
    {
        std::string want(1, static_cast<char>(0xFF));
        expect_eq("%FF", KRDecodeURLComponent("%FF"), want);
    }

    // leftover: invalid %GG kept as literal '%'.
    expect_eq("%GG", KRDecodeURLComponent("%GG"), "%GG");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
