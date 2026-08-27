// Host unit test for leftover KRScrollerView::SetContentOffset split OOB.
//
// Leftover:
//   auto splits = SplitString(value->toString(), ' ');
//   offset_x = splits[0]->toFloat();
//   offset_y = splits[1]->toFloat();
//   animate  = splits[2]->toBool();   // no size check
//
// iOS css_contentOffsetWithParams:
//   animated = [points count] > 2 ? [points[2] boolValue] : NO;
// so "10 20" is valid on iOS and operator[] OOB on OHOS.
// #1663 guarded inset / border / shadow, not this method.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_content_offset_parse_test.sh
//   ./run_kr_content_offset_parse_test.sh asan

#include "KRContentOffsetParse.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

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

static void leftover_index_oob(const char *name, const std::string &s) {
    const std::vector<std::string> tokens = kuikly::util::SplitContentOffsetTokens(s);
    bool threw = false;
    try {
        (void)tokens.at(0);
        (void)tokens.at(1);
        (void)tokens.at(2);
    } catch (const std::out_of_range &) {
        threw = true;
    }
    expect_true(name, threw);
}

static void expect_parse(const char *name, const std::string &value, bool want_applied, float x, float y, bool animate,
                         int duration, float damping, int curve) {
    kuikly::util::KRContentOffsetArgs args;
    args.offset_x = -1.f;
    args.offset_y = -1.f;
    args.animate = true;
    args.duration = -1;
    args.damping = -1.f;
    args.curve = -1;
    bool applied = false;
    try {
        applied = kuikly::util::ParseContentOffset(value, args);
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
        if (!floats_eq(args.offset_x, -1.f) || !floats_eq(args.offset_y, -1.f) || args.animate != true ||
            args.duration != -1 || !floats_eq(args.damping, -1.f) || args.curve != -1) {
            std::fprintf(stderr, "FAIL %s: size<2 wrote outs\n", name);
            ++g_failed;
            return;
        }
        std::printf("PASS %s (skip apply, no throw)\n", name);
        return;
    }
    if (!floats_eq(args.offset_x, x) || !floats_eq(args.offset_y, y) || args.animate != animate ||
        args.duration != duration || !floats_eq(args.damping, damping) || args.curve != curve) {
        std::fprintf(stderr,
                     "FAIL %s: got (%.4f %.4f a=%d d=%d damp=%.4f c=%d) want (%.4f %.4f a=%d d=%d damp=%.4f c=%d)\n",
                     name, args.offset_x, args.offset_y, static_cast<int>(args.animate), args.duration, args.damping,
                     args.curve, x, y, static_cast<int>(animate), duration, damping, curve);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (%.4g %.4g a=%d, no throw)\n", name, args.offset_x, args.offset_y,
                static_cast<int>(args.animate));
}

int main() {
    leftover_index_oob("leftover \"\" [0] OOB", "");
    leftover_index_oob("leftover \"10\" [1] OOB", "10");
    leftover_index_oob("leftover \"10 20\" [2] OOB (iOS-valid)", "10 20");

    expect_parse("\"\" (size<2)", "", false, 0.f, 0.f, false, 0, 0.f, 0);
    expect_parse("\"10\" (size<2)", "10", false, 0.f, 0.f, false, 0, 0.f, 0);
    expect_parse("\"10 20\" iOS 2-token", "10 20", true, 10.f, 20.f, false, 0, 0.f, 0);
    expect_parse("\"10 20 1\"", "10 20 1", true, 10.f, 20.f, true, 0, 0.f, 0);
    expect_parse("\"10 20 0 200 0.5 0 2\"", "10 20 0 200 0.5 0 2", true, 10.f, 20.f, false, 200, 0.5f, 2);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
