// Host unit test for leftover KRViewContext constructor bare std::stoi.
//
// Leftover:
//   KRViewContext(const std::string& instance_id, int tag){
//       box_.instance = std::stoi(instance_id);
//   }
//
// Empty / "abc" throw std::invalid_argument; oversized digits throw
// std::out_of_range. Uncaught → process abort. Sibling KRRenderView
// uses atoi (no throw). No prior leftover PR covers this constructor.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_view_context_stoi_test.sh
//   ./run_kr_view_context_stoi_test.sh asan

#include "KRViewContext.h"

#include <cstdio>
#include <stdexcept>
#include <string>

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
    } catch (const std::out_of_range &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover stoi threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
}

static void expect_parse(const char *name, const std::string &id, int want) {
    int got = -12345;
    try {
        got = ParseInstanceId(id);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
        return;
    }
    try {
        KRViewContext ctx(id, 7);
        if (ctx.Instance() != want || ctx.Tag() != 7) {
            std::fprintf(stderr, "FAIL %s: ctor Instance=%d Tag=%d\n", name, ctx.Instance(), ctx.Tag());
            ++g_failed;
            return;
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: KRViewContext ctor threw: %s\n", name, e.what());
        ++g_failed;
        return;
    }
    std::printf("PASS %s (instance=%d, no throw)\n", name, got);
}

int main() {
    leftover_stoi_throws("leftover stoi(\"\")", "");
    leftover_stoi_throws("leftover stoi(\"abc\")", "abc");
    leftover_stoi_throws("leftover stoi(overflow)", "9999999999999999999");

    expect_parse("\"\"", "", 0);
    expect_parse("\"abc\"", "abc", 0);
    expect_parse("overflow", "9999999999999999999", 0);
    expect_parse("\"42\"", "42", 42);
    expect_parse("\"12x\"", "12x", 12);

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
