#ifndef CORE_RENDER_OHOS_TEST_KR_VIEW_SUPERTOUCH_DISABLE_HOST_H
#define CORE_RENDER_OHOS_TEST_KR_VIEW_SUPERTOUCH_DISABLE_HOST_H

// Extracted leftover KRView superTouch disable-after-latch.
//
// Leftover:
//   SetProp/ResetProp superTouch false nulls handler only; type_ stays SELF
//   after EnsureSuperTouchType latched it. ProcessTouchEvent SELF branch
//   does super_touch_handler_->GetStopPropagation with no null check.
//
// Production fix:
//   disable also sets type = UNKNOWN and clears parent weak handler
//   Process SELF null-checks handler before ->
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

enum SuperTouchHostType { kUnknown, kNone, kSelf, kParent };

struct SuperTouchHostState {
    void *handler;
    SuperTouchHostType type;
    bool parent_reset;

    SuperTouchHostState() : handler(nullptr), type(kUnknown), parent_reset(false) {}
};

inline void LatchSelf(SuperTouchHostState &s, void *live_handler) {
    s.handler = live_handler;
    s.type = kSelf;
}

// leftover disable: null handler only (type stays SELF) — used to prove
// Process must not deref null even if type is still SELF.
inline void ClearHandlerOnly(SuperTouchHostState &s) {
    s.handler = nullptr;
}

// production disable (SetProp/ResetProp superTouch false)
inline void DisableSuperTouch(SuperTouchHostState &s) {
    s.handler = nullptr;
    s.type = kUnknown;
    s.parent_reset = true;
}

// ProcessTouchEvent SELF branch after the null-check fix.
// Returns true if it would have called GetStopPropagation.
// Sets would_deref_null if the leftover unguarded -> would have fired.
inline bool ProcessSelfStopPropagation(const SuperTouchHostState &s, bool &would_deref_null) {
    would_deref_null = false;
    if (s.type != kSelf) {
        return false;
    }
    if (!s.handler) {
        would_deref_null = true;  // leftover would have deref'd here
        return false;             // fixed: skip
    }
    return true;
}

#endif  // CORE_RENDER_OHOS_TEST_KR_VIEW_SUPERTOUCH_DISABLE_HOST_H
