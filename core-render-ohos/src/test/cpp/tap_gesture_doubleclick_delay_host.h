#ifndef CORE_RENDER_OHOS_TEST_TAP_GESTURE_DOUBLECLICK_DELAY_HOST_H
#define CORE_RENDER_OHOS_TEST_TAP_GESTURE_DOUBLECLICK_DELAY_HOST_H

#include <functional>
#include <memory>

// Extracted leftover KRTapGestureEventHandler double-click 250ms delay.
//
// Leftover (OnGestureEvent doubleClick ACCEPT, current_tap_count_==1):
//   KRMainThread::RunOnMainThread([this, event] { ... current_tap_count_
//     gesture_callback_ / Reset() ... }, 250)
//   UnregisterGestureEvent drops the last shared_ptr; delay has no cancel.
//   Timer then touches members on a freed handler (raw-this UAF).
//   `event` is unused and dangling after the gesture call returns.
//
// Production (no ArkUI / KRMainThread in this helper):
//   capture weak_from_this (static_pointer_cast then weak)
//   lock before any member touch; return if expired
//   do not capture event
//   live path: count==1 fires click then Reset
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

struct TapDelayHostHandler : std::enable_shared_from_this<TapDelayHostHandler> {
    int current_tap_count = 0;
    bool click_fired = false;
    bool reset_ran = false;
    bool *dtor_ran = nullptr;

    ~TapDelayHostHandler() {
        if (dtor_ran != nullptr) {
            *dtor_ran = true;
        }
    }

    void Reset() {
        current_tap_count = 0;
        reset_ran = true;
    }

    // Models the delayed body after weak lock.
    void FireDelayedClickIfSingle() {
        if (current_tap_count == 1) {
            click_fired = true;
        }
        Reset();
    }

    std::weak_ptr<TapDelayHostHandler> WeakSelf() {
        return std::weak_ptr<TapDelayHostHandler>(
            std::static_pointer_cast<TapDelayHostHandler>(shared_from_this()));
    }
};

// 250ms-style delayed callback: weak capture, lock-or-return.
inline std::function<void()> ScheduleTapDelay(const std::shared_ptr<TapDelayHostHandler> &handler) {
    auto weak_self = handler->WeakSelf();
    return [weak_self] {
        auto self = weak_self.lock();
        if (!self) {
            return;
        }
        self->FireDelayedClickIfSingle();
    };
}

#endif  // CORE_RENDER_OHOS_TEST_TAP_GESTURE_DOUBLECLICK_DELAY_HOST_H
