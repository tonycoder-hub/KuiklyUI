// Host unit test for leftover KRNodeAnimation::parseAnimation split OOB + bare stoi/stof.
//
// Leftover:
//   auto animationSpilt = kuikly::util::ConvertSplit(animation, " ");
//   animationType = std::stoi(animationSpilt[ANIMATION_TYPE_INDEX]); // 0
//   ...
//   velocity = std::stof(animationSpilt[VELOCITY_INDEX]); // 4
//   // only DELAY_INDEX / REPEAT_INDEX / ANIMATION_KEY_INDEX had size() guards
//
// Short strings ("", "0", "1 2", "a b c") OOB-index or throw std::invalid_argument.
// Non-numeric 5-token strings ("x y z q w") throw on bare stoi/stof.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_node_animation_parse_test.sh
//   ./run_kr_node_animation_parse_test.sh asan

#include "KRNodeAnimationParse.h"

#include <cstdio>
#include <stdexcept>
#include <string>

static int g_failed = 0;

static bool is_default(const kuikly::util::KRNodeAnimationParsed &p) {
    return p.animationType == 0 && p.timingFuncType == 0 && p.duration == 0.f && p.damping == 0.f && p.velocity == 0.f &&
           p.delay == 0.f && !p.repeatForever && p.animationKey.empty();
}

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

static kuikly::util::KRNodeAnimationParsed parse_no_throw(const char *name, const std::string &animation) {
    try {
        return kuikly::util::ParseNodeAnimation(animation);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return {};
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return {};
    }
}

static void expect_defaults(const char *name, const std::string &animation) {
    const auto parsed = parse_no_throw(name, animation);
    if (!is_default(parsed)) {
        std::fprintf(stderr,
                     "FAIL %s: expected defaults, got type=%d timing=%d duration=%f damping=%f velocity=%f delay=%f "
                     "repeat=%d key=%s\n",
                     name, parsed.animationType, parsed.timingFuncType, static_cast<double>(parsed.duration),
                     static_cast<double>(parsed.damping), static_cast<double>(parsed.velocity),
                     static_cast<double>(parsed.delay), static_cast<int>(parsed.repeatForever),
                     parsed.animationKey.c_str());
        ++g_failed;
        return;
    }
    std::printf("PASS %s (defaults, no throw)\n", name);
}

int main() {
    // Document leftover abort polarity: bare stoi/stof throw on these tokens.
    leftover_stoi_throws("leftover stoi(\"\")", "");
    leftover_stoi_throws("leftover stoi(\"x\")", "x");
    leftover_stof_throws("leftover stof(\"w\")", "w");

    // Required leftover inputs: no throw and leave member defaults.
    expect_defaults("empty", "");
    expect_defaults("\"1 2\"", "1 2");
    expect_defaults("\"0\"", "0");
    expect_defaults("\"a b c\"", "a b c");

    // Non-numeric 5-token leftover: no throw; skip bad fields (keep 0).
    {
        const char *name = "\"x y z q w\"";
        const auto parsed = parse_no_throw(name, "x y z q w");
        if (is_default(parsed)) {
            std::printf("PASS %s (no throw, defaults)\n", name);
        } else {
            std::fprintf(stderr, "FAIL %s: expected defaults after bad tokens\n", name);
            ++g_failed;
        }
    }

    // Valid 5-token production string.
    {
        const char *name = "\"1 2 3 4 5\"";
        const auto parsed = parse_no_throw(name, "1 2 3 4 5");
        const bool ok = parsed.animationType == 1 && parsed.timingFuncType == 2 && parsed.duration == 3.f &&
                        parsed.damping == 4.f && parsed.velocity == 5.f && parsed.delay == 0.f && !parsed.repeatForever &&
                        parsed.animationKey.empty();
        expect_true(name, ok);
        if (ok) {
            std::printf("  type=1 timing=2 duration=3 damping=4 velocity=5\n");
        }
    }

    // Full 8-token string also sets delay / repeat / key.
    {
        const char *name = "\"1 2 3 4 5 6 1 mykey\"";
        const auto parsed = parse_no_throw(name, "1 2 3 4 5 6 1 mykey");
        const bool ok = parsed.animationType == 1 && parsed.timingFuncType == 2 && parsed.duration == 3.f &&
                        parsed.damping == 4.f && parsed.velocity == 5.f && parsed.delay == 6.f && parsed.repeatForever &&
                        parsed.animationKey == "mykey";
        expect_true(name, ok);
        if (ok) {
            std::printf("  delay=6 repeatForever=true key=mykey\n");
        }
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
