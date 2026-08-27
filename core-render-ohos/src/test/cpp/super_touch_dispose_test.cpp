// Host unit test for leftover SuperTouchHandler dispose-notify raw this + null.
//
// Build + run (from this directory, no Harmony device):
//   ./run_super_touch_dispose_test.sh
//   ./run_super_touch_dispose_test.sh asan

#include "super_touch_dispose_host.h"

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
    {
        auto handler = std::make_shared<SuperTouchHostHandler>();
        handler->Collect(nullptr);
        expect_true("Collect(nullptr) is no-op", handler->gesture_recognizers_.empty());
    }

    {
        bool dtor_ran = false;
        std::function<void(void *)> dispose;
        int dummy = 1;
        {
            auto handler = std::make_shared<SuperTouchHostHandler>();
            handler->dtor_ran = &dtor_ran;
            handler->Collect(&dummy);
            dispose = handler->DisposeNotify();
            handler.reset();
        }
        expect_true("last shared_ptr dropped before dispose", dtor_ran);
        dispose(&dummy);
        expect_true("dispose after drop does not touch freed this", true);
    }

    {
        auto handler = std::make_shared<SuperTouchHostHandler>();
        int dummy = 2;
        handler->Collect(&dummy);
        auto dispose = handler->DisposeNotify();
        dispose(&dummy);
        expect_true("live dispose erases recognizer", handler->gesture_recognizers_.empty());
        expect_true("live dispose counted erase", handler->dispose_erases == 1);
    }

    {
        auto handler = std::make_shared<SuperTouchHostHandler>();
        handler->InsertRawForSkipTest(nullptr);
        handler->PreventTouch(true);
        expect_true("PreventTouch skips null recognizer", handler->enable_calls == 0);
    }

    {
        bool dtor_ran = false;
        {
            auto handler = std::make_shared<SuperTouchHostHandler>();
            handler->dtor_ran = &dtor_ran;
            handler->InsertRawForSkipTest(nullptr);
            handler.reset();
        }
        expect_true("dtor skips null recognizer", dtor_ran);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover super touch dispose tests passed\n");
    return 0;
}
