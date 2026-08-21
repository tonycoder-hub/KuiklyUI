// Host unit test for leftover KRJSONObject GetString / GetStringArray UB.
//
// cJSON_GetStringValue returns NULL for non-string items. Leftover
// GetString/GetStringArray constructed std::string from that NULL (UB/crash).
// The adopt/get-string helper is host-testable without Harmony (wraps cJSON
// or a stub that returns nullptr for non-strings).
//
// Build + run (from this directory):
//   ./run_kr_json_object_getstring_test.sh
//   ./run_kr_json_object_getstring_test.sh asan

#include "KRJSONObject.h"
#include "thirdparty/cJSON/cJSON.h"

#include <cstdio>
#include <string>
#include <vector>

using kuikly::util::AdoptCJsonGetString;
using kuikly::util::AdoptCJsonStringValue;
using kuikly::util::JSONObject;
using kuikly::util::TryAdoptCJsonStringValue;

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

static void expect_eq_size(const char *name, std::size_t got, std::size_t want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %zu want %zu\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// Tiny stub matching cJSON_GetStringValue: nullptr for non-string items.
static const char *stub_get_string_value_on_number(const void *item) {
    (void)item;
    return nullptr;
}

static const char *stub_get_string_value_on_string(const void *item) {
    return static_cast<const char *>(item);
}

int main() {
    // 1) leftover: cJSON_GetStringValue on a number item is null → helper
    //    returns default, does not crash (also via a tiny stub).
    {
        cJSON *num = cJSON_CreateNumber(1);
        const char *raw = cJSON_GetStringValue(num);
        expect_true("cJSON_GetStringValue(number) is null", raw == nullptr);
        expect_eq("AdoptCJsonStringValue(null from number)", AdoptCJsonStringValue(raw, "DEF"), "DEF");
        expect_eq("AdoptCJsonStringValue(null default empty)", AdoptCJsonStringValue(raw), "");
        cJSON_Delete(num);

        int stub_item = 1;
        expect_eq("AdoptCJsonGetString(number stub)",
                  AdoptCJsonGetString(stub_get_string_value_on_number, &stub_item, "DEF"), "DEF");
        expect_eq("AdoptCJsonGetString(string stub)",
                  AdoptCJsonGetString(stub_get_string_value_on_string, "hello", "DEF"), "hello");
        expect_true("TryAdoptCJsonStringValue(null) skips", !TryAdoptCJsonStringValue(nullptr, nullptr));
        std::string adopted;
        expect_true("TryAdoptCJsonStringValue(null) skip-element",
                    !TryAdoptCJsonStringValue(nullptr, &adopted));
    }

    // 2) leftover: GetString on {"key":1} returns default "".
    {
        auto obj = JSONObject::Parse(R"({"key":1})");
        expect_true("Parse({\"key\":1}) non-null", obj != nullptr);
        expect_eq("GetString({\"key\":1})", obj->GetString("key"), "");
        expect_eq("GetString({\"key\":1}, custom default)", obj->GetString("key", "DEF"), "DEF");
        expect_eq("GetString missing key", obj->GetString("missing", "MISS"), "MISS");
    }

    // String item still works (IsString + adopt).
    {
        auto obj = JSONObject::Parse(R"({"key":"hello"})");
        expect_eq("GetString({\"key\":\"hello\"})", obj->GetString("key"), "hello");
    }

    // 3) leftover: GetStringArray skips / does not crash on mixed/non-string.
    {
        auto obj = JSONObject::Parse(R"({"arr":["ok",1,null,true,{"x":1},"two",""]})");
        expect_true("Parse mixed array non-null", obj != nullptr);
        const std::vector<std::string> arr = obj->GetStringArray("arr");
        expect_eq_size("GetStringArray mixed size", arr.size(), 3);
        expect_eq("GetStringArray[0]", arr.size() > 0 ? arr[0] : "", "ok");
        expect_eq("GetStringArray[1]", arr.size() > 1 ? arr[1] : "", "two");
        expect_eq("GetStringArray[2] empty string kept", arr.size() > 2 ? arr[2] : "MISSING", "");
        const std::vector<std::string> missing = obj->GetStringArray("nope");
        expect_eq_size("GetStringArray missing key", missing.size(), 0);
    }

    // 4) Parse("{") is nullptr; callers must not deref.
    {
        auto bad = JSONObject::Parse("{");
        expect_true("Parse(\"{\") is nullptr", bad == nullptr);
        // Call-site contract: only use GetString after a non-null Parse.
        if (bad) {
            std::fprintf(stderr, "FAIL callers must not deref Parse nullptr\n");
            ++g_failed;
        } else {
            std::printf("PASS callers must not deref Parse nullptr\n");
        }
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
