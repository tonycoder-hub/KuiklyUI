#ifndef CORE_RENDER_OHOS_TEST_APNG_ANIMATE_DTOR_RELEASE_H
#define CORE_RENDER_OHOS_TEST_APNG_ANIMATE_DTOR_RELEASE_H

#include <memory>

// Extracted leftover APNGAnimateView dtor off-thread release:
//   KRGCDQueue::GetInstance().DispatchAsync([apng = apng_] { apng->width; });
//
// apng_ defaults nullptr. LoadFailure never assigns it. KRApngView::OnDestroy
// does Destroy(); apng_view_ = nullptr which drops the last shared_ptr and
// runs this dtor — same on LoadFile replace before FetchAPNG completes, or
// http(s) LoadFailure. Unconditional apng->width is a null deref / crash.
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

template <typename T>
inline int APNGAnimateDtorRelease(const std::shared_ptr<T> &apng) {
    if (!apng) {
        return 0;  // no-op
    }
    return apng->width;  // sub thread gc
}

#endif  // CORE_RENDER_OHOS_TEST_APNG_ANIMATE_DTOR_RELEASE_H
