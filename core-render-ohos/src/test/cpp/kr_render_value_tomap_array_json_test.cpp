// Host unit test for leftover KRRenderValue::toMap() array-JSON nullptr crash.
//
// Leftover (string path):
//   cJSON_Parse, return empty only if parse-null. Never cJSON_IsObject.
//   Array children have item->string == nullptr.
//   Map is unordered_map<std::string, ...> so map[item->string] is
//   std::string(nullptr) UB. libstdc++ throws; uncaught in render process.
//
// Fix:
//   after successful parse, if (!cJSON_IsObject) { Delete; return Map(); }
//   in the child loop, skip item->string == nullptr
//
// KRRenderValue.h cannot host-compile without Harmony. Helper
// KRRenderValueToMapHost.h mirrors the FIXED string path using real cJSON.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_render_value_tomap_array_json_test.sh
//   ./run_kr_render_value_tomap_array_json_test.sh asan

#include "KRRenderValueToMapHost.h"

#include "cJSON.h"

#include <cstdio>
#include <string>

using kuikly::leftover_tomap::toMapFromJsonString;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// leftover polarity: array JSON children have item->string == nullptr.
// map[item->string] would be std::string(nullptr) UB.
static void leftover_array_json_null_key_polarity() {
    cJSON *arr = cJSON_Parse("[1,2,3]");
    expect_true("leftover [1,2,3] parses", arr != nullptr);
    expect_true("leftover [1,2,3] is not object", arr != nullptr && !cJSON_IsObject(arr));
    expect_true("leftover array child string is nullptr",
                arr != nullptr && arr->child != nullptr && arr->child->string == nullptr);
    if (arr != nullptr) {
        cJSON_Delete(arr);
    }

    cJSON *obj_arr = cJSON_Parse("[{}]");
    expect_true("leftover [{}] parses", obj_arr != nullptr);
    expect_true("leftover [{}] is not object", obj_arr != nullptr && !cJSON_IsObject(obj_arr));
    expect_true("leftover [{}] child string is nullptr",
                obj_arr != nullptr && obj_arr->child != nullptr && obj_arr->child->string == nullptr);
    if (obj_arr != nullptr) {
        cJSON_Delete(obj_arr);
    }

    cJSON *empty_arr = cJSON_Parse("[]");
    expect_true("leftover [] parses", empty_arr != nullptr);
    expect_true("leftover [] is not object", empty_arr != nullptr && !cJSON_IsObject(empty_arr));
    if (empty_arr != nullptr) {
        cJSON_Delete(empty_arr);
    }
}

int main() {
    leftover_array_json_null_key_polarity();

    // leftover: array JSON must not throw; result is empty map.
    {
        auto m = toMapFromJsonString("[]");
        expect_true("[] -> empty map", m.empty());
    }
    {
        auto m = toMapFromJsonString("[{}]");
        expect_true("[{}] -> empty map", m.empty());
    }
    {
        auto m = toMapFromJsonString("[1,2,3]");
        expect_true("[1,2,3] -> empty map", m.empty());
    }

    // leftover: parse-null still empty (unchanged).
    {
        auto m = toMapFromJsonString("not-json");
        expect_true("not-json -> empty map", m.empty());
    }

    // leftover: object JSON still yields keys.
    {
        auto m = toMapFromJsonString("{\"a\":1}");
        expect_true("{\"a\":1} -> key a present", m.find("a") != m.end());
        expect_true("{\"a\":1} -> size 1", m.size() == 1);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover toMap array-JSON tests passed\n");
    return 0;
}
