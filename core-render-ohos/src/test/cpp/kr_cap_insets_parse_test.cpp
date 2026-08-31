// Host unit test for leftover KRImageView::SetCapInsets bare std::stof.
//
// Leftover:
//   items = ConvertSplit(valueStr, " ");
//   if (items.size() >= 4) {
//       cap_insets_* = std::stof(items[0..3]);
//   }
//
// Size is already checked (unlike #1663), but non-numeric tokens
// ("a b c d", "1 x 3 4") still throw std::invalid_argument. Policy: on
// stof failure that inset is 0; size < 4 skips apply (same as today).
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_cap_insets_parse_test.sh
//   ./run_kr_cap_insets_parse_test.sh asan

#include "KRCapInsetsParse.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

static int g_failed = 0;

static bool floats_eq(float a, float b) {
    return std::fabs(a - b) <= 0.0001f;
}

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void leftover_stof_throws(const char *name, const std::string &s) {
    bool threw = false;
    try {
        (void)std::stof(s);
    } catch (const std::invalid_argument &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover stof threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
}

static void expect_parse(const char *name, const std::string &value, bool want_applied, float top, float left,
                         float bottom, float right) {
    kuikly::util::KRCapInsetsValue insets;
    insets.top = -1.f;
    insets.left = -1.f;
    insets.bottom = -1.f;
    insets.right = -1.f;
    bool applied = false;
    try {
        applied = kuikly::util::ParseCapInsets4(value, insets);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (applied != want_applied) {
        std::fprintf(stderr, "FAIL %s: applied=%d want %d\n", name, static_cast<int>(applied),
                     static_cast<int>(want_applied));
        ++g_failed;
        return;
    }
    if (!want_applied) {
        if (!floats_eq(insets.top, -1.f) || !floats_eq(insets.left, -1.f) || !floats_eq(insets.bottom, -1.f) ||
            !floats_eq(insets.right, -1.f)) {
            std::fprintf(stderr, "FAIL %s: size<4 wrote outs top=%.4f left=%.4f bottom=%.4f right=%.4f\n", name,
                         insets.top, insets.left, insets.bottom, insets.right);
            ++g_failed;
            return;
        }
        std::printf("PASS %s (skip apply, no throw)\n", name);
        return;
    }
    if (!floats_eq(insets.top, top) || !floats_eq(insets.left, left) || !floats_eq(insets.bottom, bottom) ||
        !floats_eq(insets.right, right)) {
        std::fprintf(stderr, "FAIL %s: got (%.4f %.4f %.4f %.4f) want (%.4f %.4f %.4f %.4f)\n", name, insets.top,
                     insets.left, insets.bottom, insets.right, top, left, bottom, right);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (%.4g %.4g %.4g %.4g, no throw)\n", name, insets.top, insets.left, insets.bottom,
                insets.right);
}

int main() {
    leftover_stof_throws("leftover stof(\"a\")", "a");
    leftover_stof_throws("leftover stof(\"x\")", "x");

    // Required leftover inputs: no invalid_argument abort.
    expect_parse("\"a b c d\"", "a b c d", true, 0.f, 0.f, 0.f, 0.f);
    expect_parse("\"1 x 3 4\"", "1 x 3 4", true, 1.f, 0.f, 3.f, 4.f);
    expect_parse("\"1 2 3\" (size<4)", "1 2 3", false, 0.f, 0.f, 0.f, 0.f);
    expect_parse("\"1 2 3 4\"", "1 2 3 4", true, 1.f, 2.f, 3.f, 4.f);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
