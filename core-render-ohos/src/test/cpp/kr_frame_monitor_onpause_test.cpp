// Host unit test for leftover KRFrameMonitor::OnPause is_resumed_ polarity.
//
// Production OnPause copied OnResume's guard (`!is_started_ || is_resumed_`).
// While monitoring (started && resumed), pause returned early and never
// cleared is_resumed_. The inverse is `!is_started_ || !is_resumed_`.
//
// KRFrameMonitorFlags.h is header-only (no Harmony NativeVSync), so host
// g++/clang++ is enough. Start then OnPause must set is_resumed = false
// (that assertion fails on the leftover polarity).
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_frame_monitor_onpause_test.sh
//   ./run_kr_frame_monitor_onpause_test.sh asan

#include "libohos_render/performance/frame/KRFrameMonitorFlags.h"

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
    // Required leftover polarity: Start then OnPause clears is_resumed.
    {
        KRFrameMonitorFlags flags;
        expect_true("Start transitions", flags.Start());
        expect_true("after Start, is_started", flags.is_started);
        expect_true("after Start, is_resumed", flags.is_resumed);
        expect_true("after Start, ShouldRequestFrames", flags.ShouldRequestFrames());

        expect_true("OnPause transitions", flags.OnPause());
        expect_true("Start then OnPause: is_resumed becomes false", !flags.is_resumed);
        expect_true("Start then OnPause: still started", flags.is_started);
        expect_true("Start then OnPause: stop requesting frames", !flags.ShouldRequestFrames());
        expect_true("Start then OnPause: last_frame_time_nanos cleared",
                    flags.last_frame_time_nanos == 0);
    }

    // Extra guards: skip when not started / already paused; resume after pause.
    {
        KRFrameMonitorFlags flags;
        expect_true("OnPause before Start is a no-op", !flags.OnPause());
        expect_true("OnPause before Start leaves is_resumed false", !flags.is_resumed);

        flags.Start();
        flags.OnPause();
        expect_true("second OnPause is a no-op", !flags.OnPause());
        expect_true("second OnPause keeps is_resumed false", !flags.is_resumed);

        expect_true("OnResume after pause transitions", flags.OnResume());
        expect_true("OnResume after pause: is_resumed true", flags.is_resumed);
        expect_true("OnResume after pause: ShouldRequestFrames", flags.ShouldRequestFrames());
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
