// Host unit test for leftover canvas SetFont / processColorStops stoi/stof.
//
// Leftover:
//   SetFont:           auto weight = std::stoi(paramObj->GetString("weight"));
//   processColorStops: locations.push_back(std::stof(colorAndStop[1]));
//
// Empty / non-numeric tokens ("", "bold", location "abc" from "#f00 abc")
// throw std::invalid_argument. Helpers keep the prior weight / skip that stop.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_canvas_numeric_parse_test.sh
//   ./run_kr_canvas_numeric_parse_test.sh asan

#include "KRCanvasNumericParse.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void leftover_stoi_throws(const char *name, const std::string &s) {
    bool threw = false;
    try {
        (void)std::stoi(s);
    } catch (const std::invalid_argument &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover stoi threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
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

static void expect_weight_fail_keep_prior(const char *name, const std::string &s, int prior) {
    int weight = prior;
    bool ok = true;
    try {
        ok = kuikly::util::ParseCanvasFontWeight(s, weight);
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
        std::fprintf(stderr, "FAIL %s: expected parse fail, got weight=%d\n", name, weight);
        ++g_failed;
        return;
    }
    if (weight != prior) {
        std::fprintf(stderr, "FAIL %s: prior weight %d overwritten to %d\n", name, prior, weight);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (keep prior %d, no throw)\n", name, prior);
}

static void expect_weight_ok(const char *name, const std::string &s, int want) {
    int weight = -1;
    bool ok = false;
    try {
        ok = kuikly::util::ParseCanvasFontWeight(s, weight);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (!ok || weight != want) {
        std::fprintf(stderr, "FAIL %s: ok=%d weight=%d want %d\n", name, static_cast<int>(ok), weight, want);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (weight=%d)\n", name, want);
}

static void expect_stops(const char *name, const std::string &color_stops, size_t want) {
    std::vector<std::string> colors;
    std::vector<float> locations;
    try {
        kuikly::util::ProcessCanvasColorStops(color_stops, colors, locations);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (colors.size() != want || locations.size() != want) {
        std::fprintf(stderr, "FAIL %s: colors=%zu locations=%zu want %zu\n", name, colors.size(), locations.size(),
                     want);
        ++g_failed;
        return;
    }
    std::printf("PASS %s (stops=%zu, no throw)\n", name, want);
}

int main() {
    // Document leftover abort polarity: bare stoi/stof throw on these tokens.
    leftover_stoi_throws("leftover stoi(\"\")", "");
    leftover_stoi_throws("leftover stoi(\"bold\")", "bold");
    leftover_stof_throws("leftover stof(\"abc\") from #f00 abc", "abc");

    // Required leftover inputs: no invalid_argument abort; defaults / skip.
    expect_weight_fail_keep_prior("weight \"\"", "", 400);
    expect_weight_fail_keep_prior("weight \"bold\"", "bold", 700);
    expect_stops("location \"#f00 abc\"", "#f00 abc", 0);

    // Valid leftover-adjacent production tokens still parse.
    expect_weight_ok("weight \"400\"", "400", 400);
    expect_weight_ok("weight \"700\"", "700", 700);
    expect_stops("valid #f00 0", "#f00 0", 1);
    expect_stops("valid production pair", "4294901760 0.0,4278255360 1.0", 2);

    // Mixed leftover: skip only the bad stop, keep the rest aligned.
    expect_stops("mixed skip abc", "#f00 0,#0f0 abc,#00f 1", 2);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
