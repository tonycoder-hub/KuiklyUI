// Host unit test for leftover KRLinearGradientParser stoi/stof on malformed CSS.
//
// ParseFromCssLinearGradient used to:
//   - underflow size_t when the string lacked a trailing ')'
//   - throw on std::stoi("") / std::stof("#ff0000") for empty or comma+space tokens
//   - return true even when no colors were parsed
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_linear_gradient_parser_test.sh
//   ./run_kr_linear_gradient_parser_test.sh asan

#include "KRLinearGradientCssParse.h"

#include <cstdio>
#include <stdexcept>
#include <string>

static int g_failed = 0;

static void expect_false_no_throw(const char *name, const std::string &css) {
    kuikly::util::KRLinearGradientCssParser parser;
    bool ok = true;
    try {
        ok = parser.ParseFromCssLinearGradient(css);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (ok) {
        std::fprintf(stderr, "FAIL %s: expected false, got true\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s (false, no throw)\n", name);
    }
}

static void expect_true_no_throw(const char *name, const std::string &css, int want_dir, size_t want_stops) {
    kuikly::util::KRLinearGradientCssParser parser;
    bool ok = false;
    try {
        ok = parser.ParseFromCssLinearGradient(css);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (!ok) {
        std::fprintf(stderr, "FAIL %s: expected true, got false\n", name);
        ++g_failed;
        return;
    }
    if (parser.GetDirection() != want_dir) {
        std::fprintf(stderr, "FAIL %s: direction %d want %d\n", name, parser.GetDirection(), want_dir);
        ++g_failed;
        return;
    }
    if (parser.GetColors().size() != want_stops || parser.GetLocations().size() != want_stops) {
        std::fprintf(stderr, "FAIL %s: stops %zu/%zu want %zu\n", name, parser.GetColors().size(),
                     parser.GetLocations().size(), want_stops);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (true, dir=%d, stops=%zu)\n", name, want_dir, want_stops);
}

int main() {
    // Leftover: empty body still had trailing ')' but stoi("") threw.
    expect_false_no_throw("linear-gradient()", "linear-gradient()");

    // Leftover: missing ')' underflowed size_t in substr length math.
    expect_false_no_throw("linear-gradient(", "linear-gradient(");

    // Other malformed inputs: no throw, false.
    expect_false_no_throw("not-a-gradient", "not-a-gradient");
    expect_false_no_throw("linear-gradient(abc)", "linear-gradient(abc)");
    expect_false_no_throw("linear-gradient(0)", "linear-gradient(0)");
    expect_false_no_throw("linear-gradient(0, #ff0000 xyz)", "linear-gradient(0, #ff0000 xyz)");
    expect_false_no_throw("empty color token", "linear-gradient(0,  , #ff0000 0)");

    // Valid CSS-like form (comma+space after direction; no space after later commas).
    expect_true_no_throw("valid #hex stops", "linear-gradient(0, #ff0000 0,#00ff00 1)", 0, 2);

    // Production-like decimal color + space after comma must not stof the hex/color token.
    expect_true_no_throw("valid comma+space", "linear-gradient(0, #ff0000 0, #00ff00 1)", 0, 2);

    // Kotlin ColorStop.toString() emits decimal ARGB + stop, no spaces after commas.
    expect_true_no_throw("valid production decimal", "linear-gradient(0,4294901760 0,4278255360 1)", 0, 2);
    expect_true_no_throw("valid production float stops", "linear-gradient(0,4294901760 0.0,4278255360 1.0)", 0, 2);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
