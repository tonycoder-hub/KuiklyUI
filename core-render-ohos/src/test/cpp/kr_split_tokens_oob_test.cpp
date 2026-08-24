// Host unit test for leftover ConvertSplit / SplitString OOB on short props.
//
// ConverToBorderRadiuses / content-inset / boxShadow / border indexed
// splits[0]..[3] after ConvertSplit / SplitString. Short strings ("8", "",
// "10 20", "1 2 3") produce <4 tokens. Helpers pad-to-4 with "0" for
// radiuses/inset, and skip shadow/border when the token count is short.
// Header-only KRSplitTokens.h so this compiles without ArkUI / Harmony.
//
// Build + run (from this directory):
//   ./run_kr_split_tokens_oob_test.sh
//   ./run_kr_split_tokens_oob_test.sh asan

#include "libohos_render/foundation/KRBorderRadiuses.h"
#include "libohos_render/utils/KRSplitTokens.h"

#include <cmath>
#include <cstdio>
#include <string>

using kuikly::util::BorderParts;
using kuikly::util::BoxShadowParts;
using kuikly::util::ContentInsetParts;
using kuikly::util::ConverToBorderRadiuses;
using kuikly::util::ParseContentInsetParts;
using kuikly::util::TryParseBorderParts;
using kuikly::util::TryParseBoxShadowParts;

static int g_failed = 0;

static bool feq(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_radius(const char *name, const KRBorderRadiuses &got, float tl, float tr, float bl, float br) {
    if (feq(got.topLeft, tl) && feq(got.topRight, tr) && feq(got.bottomLeft, bl) && feq(got.bottomRight, br)) {
        std::printf("PASS %s\n", name);
    } else {
        std::fprintf(stderr, "FAIL %s: got %g,%g,%g,%g want %g,%g,%g,%g\n", name, got.topLeft, got.topRight,
                     got.bottomLeft, got.bottomRight, tl, tr, bl, br);
        ++g_failed;
    }
}

static void expect_inset(const char *name, const ContentInsetParts &got, float top, float start, float bottom,
                         float end, bool animate) {
    if (feq(got.top, top) && feq(got.start, start) && feq(got.bottom, bottom) && feq(got.end, end) &&
        got.animate == animate) {
        std::printf("PASS %s\n", name);
    } else {
        std::fprintf(stderr, "FAIL %s: got %g,%g,%g,%g animate=%d want %g,%g,%g,%g animate=%d\n", name, got.top,
                     got.start, got.bottom, got.end, got.animate ? 1 : 0, top, start, bottom, end, animate ? 1 : 0);
        ++g_failed;
    }
}

int main() {
    // leftover: ConverToBorderRadiuses("8") used to index splits[1]..[3] OOB.
    // pad-to-4 with "0" → 8,0,0,0 (defined defaults, no crash).
    expect_radius("ConverToBorderRadiuses(\"8\")", ConverToBorderRadiuses("8"), 8, 0, 0, 0);

    // leftover: ConverToBorderRadiuses("") was ConvertSplit → [""] then [1]..[3] OOB.
    expect_radius("ConverToBorderRadiuses(\"\")", ConverToBorderRadiuses(""), 0, 0, 0, 0);

    // full 4-token path unchanged
    expect_radius("ConverToBorderRadiuses(\"1,2,3,4\")", ConverToBorderRadiuses("1,2,3,4"), 1, 2, 3, 4);
    expect_radius("ConverToBorderRadiuses(\"8,16\")", ConverToBorderRadiuses("8,16"), 8, 16, 0, 0);

    // leftover: content-inset "10 20" used to index [2]..[3] OOB.
    expect_inset("content-inset \"10 20\"", ParseContentInsetParts("10 20"), 10, 20, 0, 0, false);
    expect_inset("content-inset \"\"", ParseContentInsetParts(""), 0, 0, 0, 0, false);
    expect_inset("content-inset \"1 2 3 4 1\"", ParseContentInsetParts("1 2 3 4 1"), 1, 2, 3, 4, true);
    expect_inset("content-inset \"1 2 3 4 0\"", ParseContentInsetParts("1 2 3 4 0"), 1, 2, 3, 4, false);

    // leftover: short boxShadow must skip (no [0]..[3] OOB), not invent color.
    {
        BoxShadowParts parts;
        expect_true("short boxShadow \"1 2 3\" skip", !TryParseBoxShadowParts("1 2 3", parts));
        expect_true("short boxShadow \"\" skip", !TryParseBoxShadowParts("", parts));
        expect_true("short boxShadow \"1\" skip", !TryParseBoxShadowParts("1", parts));

        expect_true("boxShadow \"1 2 3 4\" parse", TryParseBoxShadowParts("1 2 3 4", parts));
        expect_true("boxShadow x", feq(parts.x, 1));
        expect_true("boxShadow y", feq(parts.y, 2));
        expect_true("boxShadow radius", feq(parts.radius, 3));
        expect_true("boxShadow color", parts.color == "4");
        expect_true("boxShadow fill default", parts.fill);

        expect_true("boxShadow \"1 2 3 #00 0\" parse", TryParseBoxShadowParts("1 2 3 #00 0", parts));
        expect_true("boxShadow fill 0", !parts.fill);
    }

    // leftover: short border skip; 3-token reset / kotlin Border.toString stay valid.
    {
        BorderParts parts;
        expect_true("short border \"1\" skip", !TryParseBorderParts("1", parts));
        expect_true("short border \"1 solid\" skip", !TryParseBorderParts("1 solid", parts));
        expect_true("short border \"\" skip", !TryParseBorderParts("", parts));
        expect_true("border \"0 solid 0\" parse", TryParseBorderParts("0 solid 0", parts));
        expect_true("border width 0", feq(parts.width, 0));
        expect_true("border style solid", parts.style == "solid");
        expect_true("border color 0", parts.color == "0");
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
