// Host unit test for leftover APNGAnimateView dtor null-deref.
//
// Leftover:
//   APNGAnimateView::~APNGAnimateView() {
//     Destroy();
//     KRGCDQueue::GetInstance().DispatchAsync([apng = apng_] {
//         apng->width;  // sub thread gc
//     });
//   }
//
// apng_ defaults nullptr (header). LoadFailure never assigns apng_.
// KRApngView::OnDestroy does Destroy(); apng_view_ = nullptr which drops the
// last shared_ptr and runs this dtor. Same on LoadFile replace before
// FetchAPNG completes, or http(s) LoadFailure. Unconditional apng->width
// is a null deref / crash.
//
// Production fix: only dispatch / only deref when apng_ is non-null.
// Off-thread release extracted here so a null shared_ptr is a no-op.
// Header-only helper — compiles without ArkUI / Harmony.
//
// Build + run (from this directory, no Harmony device):
//   ./run_apng_animate_dtor_null_test.sh
//   ./run_apng_animate_dtor_null_test.sh asan

#include "apng_animate_dtor_release.h"

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

static void expect_eq(const char *name, int got, int want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

struct DummyAPNG {
    int width = 42;
};

int main() {
    // leftover polarity: null shared_ptr (LoadFailure / destroy-before-load)
    // used to deref apng->width → crash. Must be a no-op.
    {
        std::shared_ptr<DummyAPNG> apng;
        expect_true("null shared_ptr is empty", !apng);
        expect_eq("null shared_ptr is no-op", APNGAnimateDtorRelease(apng), 0);
    }

    {
        std::shared_ptr<DummyAPNG> apng = nullptr;
        expect_eq("explicit nullptr is no-op", APNGAnimateDtorRelease(apng), 0);
    }

    // leftover: non-null dummy with width field — deref ok (sub thread gc).
    {
        auto apng = std::make_shared<DummyAPNG>();
        apng->width = 128;
        expect_eq("non-null dummy width deref", APNGAnimateDtorRelease(apng), 128);
        expect_true("non-null dummy still alive after touch", apng.use_count() == 1);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover apng animate dtor tests passed\n");
    return 0;
}
