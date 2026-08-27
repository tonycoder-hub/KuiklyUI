// Host unit test for leftover KRTapGestureEventHandler 250ms raw-this UAF.
//
// Leftover: doubleClick ACCEPT schedules [this, event] for 250ms.
// Unregister drops last shared_ptr; timer still touches members.
//
// Production: weak capture + lock; expired is a no-op (no member touch).
// Live path: keep the shared_ptr, count==1 fires click then Reset.
//
// Build + run (from this directory, no Harmony device):
//   ./run_tap_gesture_doubleclick_delay_test.sh
//   ./run_tap_gesture_doubleclick_delay_test.sh asan

#include "tap_gesture_doubleclick_delay_host.h"

#include <cstdio>
#include <memory>

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
    // Destroy-before-fire: drop last shared_ptr, then run the delayed
    // callback. Must no-op (no member touch) — leftover raw-this UAF.
    {
        bool dtor_ran = false;
        std::function<void()> delayed;
        {
            auto handler = std::make_shared<TapDelayHostHandler>();
            handler->dtor_ran = &dtor_ran;
            handler->current_tap_count = 1;
            delayed = ScheduleTapDelay(handler);
            expect_true("handler live before drop", handler.use_count() == 1);
            handler.reset();
        }
        expect_true("last shared_ptr dropped before fire", dtor_ran);
        delayed();
        expect_true("expired delay is a no-op", true);
    }

    // Live path: keep the shared_ptr; count==1 fires click then Reset.
    {
        auto handler = std::make_shared<TapDelayHostHandler>();
        handler->current_tap_count = 1;
        auto delayed = ScheduleTapDelay(handler);
        delayed();
        expect_true("live delay fires click when count==1", handler->click_fired);
        expect_true("live delay Reset after click", handler->reset_ran);
        expect_true("live delay Reset cleared count", handler->current_tap_count == 0);
    }

    // Live path: count!=1 does not fire click, still Reset.
    {
        auto handler = std::make_shared<TapDelayHostHandler>();
        handler->current_tap_count = 2;
        auto delayed = ScheduleTapDelay(handler);
        delayed();
        expect_true("live delay skips click when count!=1", !handler->click_fired);
        expect_true("live delay Reset when count!=1", handler->reset_ran);
        expect_true("live delay Reset cleared count!=1", handler->current_tap_count == 0);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover tap gesture doubleclick delay tests passed\n");
    return 0;
}
