#ifndef CORE_RENDER_OHOS_TEST_SUPER_TOUCH_DISPOSE_HOST_H
#define CORE_RENDER_OHOS_TEST_SUPER_TOUCH_DISPOSE_HOST_H

#include <functional>
#include <memory>
#include <unordered_set>

// Extracted leftover SuperTouchHandler Collect / dispose-notify / dtor.
//
// Leftover:
//   Collect inserts GetRecognizer with no null check.
//   Dispose notify stores raw this and erases into the set.
//   dtor / PreventTouch then call SetGestureRecognizerEnabled on nullptr
//   or a dead SuperTouchHandler after last shared_ptr drop.
//
// Production (no ArkUI / Harmony in this helper):
//   Collect(nullptr) is a no-op
//   dispose notify captures weak_from_this; lock before erase; return if expired
//   PreventTouch / dtor skip null recognizers
//   dtor snapshots the set, installs empty notify, then enable+clear
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

struct SuperTouchHostHandler : std::enable_shared_from_this<SuperTouchHostHandler> {
    std::unordered_set<void *> gesture_recognizers_;
    bool prevent_touch_ = false;
    bool *dtor_ran = nullptr;
    int enable_calls = 0;
    int empty_notify_installs = 0;
    int dispose_erases = 0;

    ~SuperTouchHostHandler() {
        auto snapshot = gesture_recognizers_;
        for (auto recognizer : snapshot) {
            if (recognizer == nullptr) {
                continue;
            }
            ++empty_notify_installs;
            ++enable_calls;
        }
        gesture_recognizers_.clear();
        if (dtor_ran != nullptr) {
            *dtor_ran = true;
        }
    }

    void Collect(void *recognizer) {
        if (recognizer == nullptr) {
            return;
        }
        if (gesture_recognizers_.find(recognizer) != gesture_recognizers_.end()) {
            return;
        }
        gesture_recognizers_.insert(recognizer);
    }

    std::function<void(void *)> DisposeNotify() {
        auto weak_self = std::weak_ptr<SuperTouchHostHandler>(shared_from_this());
        return [weak_self](void *recognizer) {
            auto self = weak_self.lock();
            if (!self) {
                return;
            }
            self->gesture_recognizers_.erase(recognizer);
            ++self->dispose_erases;
        };
    }

    void PreventTouch(bool prevent) {
        if (prevent_touch_ == prevent) {
            return;
        }
        prevent_touch_ = prevent;
        for (auto recognizer : gesture_recognizers_) {
            if (recognizer == nullptr) {
                continue;
            }
            ++enable_calls;
        }
    }

    void InsertRawForSkipTest(void *recognizer) { gesture_recognizers_.insert(recognizer); }
};

#endif  // CORE_RENDER_OHOS_TEST_SUPER_TOUCH_DISPOSE_HOST_H
