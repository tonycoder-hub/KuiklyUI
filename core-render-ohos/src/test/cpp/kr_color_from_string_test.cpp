// Host unit test for leftover Color::FromString uninitialized return.
//
// KRColor.h is header-only. Empty / non-matching strings used to return an
// uninitialized Color union (indeterminate ARGB). Default is now 0
// (transparent black), matching ConvertToHexColor's parse-failure fallback.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_color_from_string_test.sh
//   ./run_kr_color_from_string_test.sh asan

// Quoted include of KRStringUtil.h from KRColor.h searches the header's own
// directory first, so the production (Harmony) header wins over -I. Include
// the host stub first; matching include guards skip the NDK-backed header.
#include "KRStringUtil.h"
#include "KRColor.h"

#include <cstdint>
#include <cstdio>
#include <string>

static int g_failed = 0;

static void expect_eq_u32(const char *name, std::uint32_t got, std::uint32_t want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got 0x%08x want 0x%08x\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s (0x%08x)\n", name, got);
    }
}

int main() {
    using kuikly::graphics::Color;

    // Leftover: empty string skipped every branch and returned uninitialized.
    const Color empty = Color::FromString("");
    expect_eq_u32("FromString(\"\")", empty.value, 0u);

    // Non-matching text takes the atoi fallback; atoi("not-a-color") is 0.
    const Color malformed = Color::FromString("not-a-color");
    expect_eq_u32("FromString(\"not-a-color\")", malformed.value, 0u);

    // rgba() with the wrong arity also used to leave ret uninitialized.
    const Color bad_rgba = Color::FromString("rgba(1,2,3)");
    expect_eq_u32("FromString(\"rgba(1,2,3)\")", bad_rgba.value, 0u);

    // Valid #RRGGBB: opaque red.
    const Color hex = Color::FromString("#FF0000");
    expect_eq_u32("FromString(\"#FF0000\")", hex.value, 0xFFFF0000u);

    // Valid rgba sample (ets-side form).
    const Color rgba = Color::FromString("rgba(255,0,0,1)");
    expect_eq_u32("FromString(\"rgba(255,0,0,1)\")", rgba.value, 0xFFFF0000u);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
