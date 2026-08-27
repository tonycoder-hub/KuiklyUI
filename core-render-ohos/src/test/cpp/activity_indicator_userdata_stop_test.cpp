// Host unit test for leftover ActivityIndicator MyUserData stop/new-free.
//
// Build + run (from this directory, no Harmony device):
//   ./run_activity_indicator_userdata_stop_test.sh
//   ./run_activity_indicator_userdata_stop_test.sh asan

#include "activity_indicator_userdata_host.h"

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

static void dispose_option(ActivityIndicatorHostUserData *user_data) {
    if (user_data != nullptr && user_data->option != nullptr) {
        delete user_data->option;
        user_data->option = nullptr;
    }
}

int main() {
    {
        ActivityIndicatorHostUserData *slot = InitMyUserData();
        expect_true("init stop defaults false", slot != nullptr && slot->stop == false);
        TeardownMyUserData(slot, dispose_option);
        expect_true("teardown nulls slot", slot == nullptr);
    }

    {
        bool dtor_ran = false;
        auto live = std::make_shared<int>(1);
        ActivityIndicatorHostUserData *slot = InitMyUserData();
        slot->dtor_ran = &dtor_ran;
        slot->weak_view = live;
        slot->option = new int(7);
        expect_true("init stop false before teardown", slot->stop == false);
        expect_true("weak_view attached", !slot->weak_view.expired());
        TeardownMyUserData(slot, dispose_option);
        expect_true("delete (not free) ran dtor", dtor_ran);
        expect_true("slot nulled after delete", slot == nullptr);
        expect_true("weak_ptr dtor released control block", live.use_count() == 1);
        live.reset();
    }

    {
        bool dtor_ran = false;
        ActivityIndicatorHostUserData *slot = InitMyUserData();
        slot->dtor_ran = &dtor_ran;
        slot->option = new int(3);
        expect_true("null animate_api is no-op", !AnimateToIfPresent(nullptr));
        OnDestroyMyUserData(slot, dispose_option);
        expect_true("unarmed OnDestroy sets stop and deletes", dtor_ran);
        expect_true("unarmed OnDestroy nulls slot", slot == nullptr);
    }

    // Armed OnDestroy detaches owner and drops the view slot. Complete
    // deletes the live MyUserData and must not write through a dangling owner
    // (the leftover unmount-while-rotating write into a destroyed view).
    {
        bool dtor_ran = false;
        ActivityIndicatorHostUserData *slot = InitMyUserData();
        slot->dtor_ran = &dtor_ran;
        slot->option = new int(5);
        AttachOwner(slot);
        int dummy_api = 1;
        ArmAnimateTo(slot, &dummy_api);
        expect_true("armed after animateTo", slot->armed);
        expect_true("owner attached to view slot", slot->owner == &slot);
        ActivityIndicatorHostUserData *raw = slot;
        OnDestroyMyUserData(slot, dispose_option);
        expect_true("armed OnDestroy does not delete", !dtor_ran);
        expect_true("armed OnDestroy drops view slot", slot == nullptr);
        expect_true("armed OnDestroy detaches owner", raw->owner == nullptr);
        expect_true("armed OnDestroy left object live", raw->stop);
        CompleteDelete(raw, dispose_option);
        expect_true("complete after detach deletes live object", dtor_ran);
    }

    {
        int dummy_api = 1;
        expect_true("non-null animate_api is not skipped", AnimateToIfPresent(&dummy_api));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover activity indicator userdata tests passed\n");
    return 0;
}
