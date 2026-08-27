// Host unit test for leftover KRView superTouch disable-after-latch null deref.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_view_supertouch_disable_test.sh
//   ./run_kr_view_supertouch_disable_test.sh asan

#include "kr_view_supertouch_disable_host.h"

#include <cstdio>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    int live = 1;

    // leftover polarity: latch SELF with live handler, then clear handler
    // without resetting type. Next Process SELF must not deref null.
    {
        SuperTouchHostState s;
        LatchSelf(s, &live);
        expect_true("latched SELF with handler", s.type == kSelf && s.handler != nullptr);
        ClearHandlerOnly(s);
        expect_true("leftover disable leaves type SELF", s.type == kSelf && s.handler == nullptr);
        bool would_deref_null = false;
        bool called = ProcessSelfStopPropagation(s, would_deref_null);
        expect_true("leftover SELF+null would have deref'd", would_deref_null);
        expect_true("fixed Process does not call through null", !called);
    }

    // After reset-type fix: disable-then-next-touch stays UNKNOWN and skips SELF.
    {
        SuperTouchHostState s;
        LatchSelf(s, &live);
        DisableSuperTouch(s);
        expect_true("disable resets type UNKNOWN", s.type == kUnknown);
        expect_true("disable nulls handler", s.handler == nullptr);
        expect_true("disable resets parent handler", s.parent_reset);
        bool would_deref_null = false;
        bool called = ProcessSelfStopPropagation(s, would_deref_null);
        expect_true("UNKNOWN skips SELF deref", !called && !would_deref_null);
    }

    // Live SELF still calls through.
    {
        SuperTouchHostState s;
        LatchSelf(s, &live);
        bool would_deref_null = false;
        bool called = ProcessSelfStopPropagation(s, would_deref_null);
        expect_true("live SELF still calls handler", called && !would_deref_null);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover superTouch disable tests passed\n");
    return 0;
}
