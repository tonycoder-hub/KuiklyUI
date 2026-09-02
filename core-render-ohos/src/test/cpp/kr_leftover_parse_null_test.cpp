// Host unit test for leftover JSONObject::Parse-null deref
// (back_press / scroller / page-create-trace / OH prefs).
//
// JSONObject::Parse returns nullptr on bad JSON. These leftover call
// sites used to deref that pointer. Thin wrappers mimic each
// Parse+use path so the harness compiles without Harmony / NAPI.
// KRPageCreateTrace.cpp has no OHOS APIs and is compiled for real.
//
// Build + run (from this directory):
//   ./run_kr_leftover_parse_null_test.sh
//   ./run_kr_leftover_parse_null_test.sh asan

#include "KRPageCreateTrace.h"
#include "KRJSONObject.h"

#include <cstdio>
#include <string>

using kuikly::util::JSONObject;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq(const char *name, const std::string &got, const std::string &want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, got.c_str(), want.c_str());
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq_i64(const char *name, int64_t got, int64_t want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %lld want %lld\n", name, static_cast<long long>(got),
                     static_cast<long long>(want));
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// Mimic KRBackPressModule::BackHandle: Parse then GetNumber("consumed").
// Leftover unguarded: jsonObj->GetNumber after a failed Parse.
static bool leftover_back_handle(const std::string &params, bool prior) {
    auto jsonObj = JSONObject::Parse(params);
    if (!jsonObj) {
        return prior;
    }
    return jsonObj->GetNumber("consumed", 0) == 1;
}

// Mimic KRScrollerView::SetNestedScroll: Parse then GetString forward/backward.
// Leftover unguarded: paramObj->GetString after a failed Parse.
struct NestedScrollAdopt {
    bool applied = false;
    std::string forward;
    std::string backward;
};

static NestedScrollAdopt leftover_set_nested_scroll(const std::string &str) {
    NestedScrollAdopt out;
    auto paramObj = JSONObject::Parse(str);
    if (!paramObj) {
        return out;
    }
    out.forward = paramObj->GetString("forward");
    out.backward = paramObj->GetString("backward");
    out.applied = true;
    return out;
}

// Mimic KROhSharedPreferencesModule::SetItem: Parse then GetString key/value.
// Leftover unguarded: jsonObj->GetString after a failed Parse.
// (KRSharedPreferencesModule was fixed in #1651; this is the OH sibling.)
struct PrefsSetItemAdopt {
    bool applied = false;
    std::string key;
    std::string value;
};

static PrefsSetItemAdopt leftover_oh_prefs_set_item(const std::string &params) {
    PrefsSetItemAdopt out;
    auto jsonObj = JSONObject::Parse(params);
    if (!jsonObj) {
        return out;
    }
    out.key = jsonObj->GetString("key");
    out.value = jsonObj->GetString("value");
    out.applied = true;
    return out;
}

int main() {
    // leftover: Parse("{") / Parse("") is nullptr. Unguarded deref would crash.
    {
        expect_true("Parse(\"{\") is nullptr", JSONObject::Parse("{") == nullptr);
        expect_true("Parse(\"\") is nullptr", JSONObject::Parse("") == nullptr);
        expect_true("Parse(\"not-json\") is nullptr", JSONObject::Parse("not-json") == nullptr);
    }

    // 1) leftover back_press BackHandle
    {
        expect_true("BackHandle(\"{\") keeps prior false", leftover_back_handle("{", false) == false);
        expect_true("BackHandle(\"\") keeps prior true", leftover_back_handle("", true) == true);
        expect_true("BackHandle({\"consumed\":1}) sets consumed", leftover_back_handle(R"({"consumed":1})", false));
        expect_true("BackHandle({\"consumed\":0}) clears consumed",
                    leftover_back_handle(R"({"consumed":0})", true) == false);
    }

    // 2) leftover scroller SetNestedScroll
    {
        const NestedScrollAdopt bad_brace = leftover_set_nested_scroll("{");
        const NestedScrollAdopt bad_empty = leftover_set_nested_scroll("");
        expect_true("SetNestedScroll(\"{\") not applied", !bad_brace.applied);
        expect_true("SetNestedScroll(\"\") not applied", !bad_empty.applied);

        const NestedScrollAdopt ok = leftover_set_nested_scroll(R"({"forward":"SELF_FIRST","backward":"SELF_ONLY"})");
        expect_true("SetNestedScroll valid applied", ok.applied);
        expect_eq("SetNestedScroll forward", ok.forward, "SELF_FIRST");
        expect_eq("SetNestedScroll backward", ok.backward, "SELF_ONLY");
    }

    // 3) leftover KRPageCreateTrace ctor (real production .cpp)
    {
        KRPageCreateTrace bad_brace("{");
        expect_eq_i64("trace(\"{\") create_start default", bad_brace.create_start_timeMills, 0);
        expect_eq_i64("trace(\"{\") create_end default", bad_brace.create_end_timeMills, 0);
        expect_eq_i64("trace(\"{\") on_build_start default", bad_brace.build_start_timeMills, 0);

        KRPageCreateTrace bad_empty("");
        expect_eq_i64("trace(\"\") create_start default", bad_empty.create_start_timeMills, 0);
        expect_eq_i64("trace(\"\") newPage_end default", bad_empty.newPage_end_timeMills, 0);

        KRPageCreateTrace ok(R"({"on_create_start":11,"on_create_end":22,"on_build_start":33})");
        expect_eq_i64("trace valid on_create_start", ok.create_start_timeMills, 11);
        expect_eq_i64("trace valid on_create_end", ok.create_end_timeMills, 22);
        expect_eq_i64("trace valid on_build_start", ok.build_start_timeMills, 33);
    }

    // 4) leftover KROhSharedPreferencesModule::SetItem
    {
        const PrefsSetItemAdopt bad_brace = leftover_oh_prefs_set_item("{");
        const PrefsSetItemAdopt bad_empty = leftover_oh_prefs_set_item("");
        expect_true("OH prefs SetItem(\"{\") not applied", !bad_brace.applied);
        expect_true("OH prefs SetItem(\"\") not applied", !bad_empty.applied);
        expect_eq("OH prefs SetItem(\"{\") key empty", bad_brace.key, "");
        expect_eq("OH prefs SetItem(\"{\") value empty", bad_brace.value, "");

        const PrefsSetItemAdopt ok = leftover_oh_prefs_set_item(R"({"key":"k","value":"v"})");
        expect_true("OH prefs SetItem valid applied", ok.applied);
        expect_eq("OH prefs SetItem key", ok.key, "k");
        expect_eq("OH prefs SetItem value", ok.value, "v");
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
