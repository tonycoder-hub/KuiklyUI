#ifndef CORE_RENDER_OHOS_TEST_ACTIVITY_INDICATOR_USERDATA_HOST_H
#define CORE_RENDER_OHOS_TEST_ACTIVITY_INDICATOR_USERDATA_HOST_H

#include <memory>

// Extracted leftover KRActivityIndicatorAnimationView MyUserData init/teardown.
//
// Production lifetime (no ArkUI in this helper):
//   stop=false on create; armed=false until animateTo
//   OnDestroy sets stop=true
//   Unarmed: OnDestroy deletes
//   Armed: OnDestroy detaches owner and drops the view slot; complete deletes
//   the live MyUserData and must not write *owner (view already gone)
//   delete not free; dispose option; null animate_api skip
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

struct ActivityIndicatorHostUserData {
    std::weak_ptr<int> weak_view;
    bool stop;
    bool armed;
    int *option;
    bool *dtor_ran;
    ActivityIndicatorHostUserData **owner;

    ActivityIndicatorHostUserData()
        : stop(false), armed(false), option(nullptr), dtor_ran(nullptr), owner(nullptr) {}

    ~ActivityIndicatorHostUserData() {
        if (dtor_ran != nullptr) {
            *dtor_ran = true;
        }
    }
};

inline ActivityIndicatorHostUserData *InitMyUserData() {
    ActivityIndicatorHostUserData *user_data = new ActivityIndicatorHostUserData();
    user_data->stop = false;
    user_data->armed = false;
    return user_data;
}

inline void AttachOwner(ActivityIndicatorHostUserData *&slot) {
    if (slot != nullptr) {
        slot->owner = &slot;
    }
}

template <typename DisposeFn>
inline void DeleteMyUserData(ActivityIndicatorHostUserData *user_data, DisposeFn dispose_option) {
    if (user_data == nullptr) {
        return;
    }
    if (user_data->owner != nullptr) {
        *user_data->owner = nullptr;
        user_data->owner = nullptr;
    }
    dispose_option(user_data);
    delete user_data;
}

template <typename DisposeFn>
inline void TeardownMyUserData(ActivityIndicatorHostUserData *&slot, DisposeFn dispose_option) {
    if (slot == nullptr) {
        return;
    }
    ActivityIndicatorHostUserData *user_data = slot;
    slot = nullptr;
    user_data->owner = nullptr;
    dispose_option(user_data);
    delete user_data;
}

template <typename DisposeFn>
inline void OnDestroyMyUserData(ActivityIndicatorHostUserData *&slot, DisposeFn dispose_option) {
    if (slot == nullptr) {
        return;
    }
    slot->stop = true;
    if (!slot->armed) {
        DeleteMyUserData(slot, dispose_option);
        slot = nullptr;
    } else {
        slot->owner = nullptr;
        slot = nullptr;
    }
}

// complete holds the raw MyUserData pointer, not the view slot.
// Must not write *owner if OnDestroy already detached it.
template <typename DisposeFn>
inline void CompleteDelete(ActivityIndicatorHostUserData *user_data, DisposeFn dispose_option) {
    if (user_data == nullptr) {
        return;
    }
    if (user_data->stop) {
        DeleteMyUserData(user_data, dispose_option);
    }
}

inline bool AnimateToIfPresent(void *animate_api) {
    if (!animate_api) {
        return false;
    }
    return true;
}

inline void ArmAnimateTo(ActivityIndicatorHostUserData *user_data, void *animate_api) {
    if (!AnimateToIfPresent(animate_api) || user_data == nullptr) {
        return;
    }
    user_data->armed = true;
}

#endif  // CORE_RENDER_OHOS_TEST_ACTIVITY_INDICATOR_USERDATA_HOST_H
