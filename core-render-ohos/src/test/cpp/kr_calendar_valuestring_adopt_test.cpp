// Host unit test for leftover KRCalendarModule valuestring adopt.
//
// Leftover CalDate/Format/Parse constructed std::string from
// item->valuestring whenever the key existed. cJSON sets valuestring only
// for string items; a number/bool/null field has valuestring == NULL.
// std::string(NULL) is UB — same adopt #1651 fixed in JSONObject::GetString.
// cJSON_Parse of bad JSON also leaves the object null; leftover CalDate then
// called GetObjectItem on null.
//
// AdoptCJsonStringValue / AdoptCalendarObjectString are NDK-free so this
// runs on host g++/clang++ without a Harmony device.
//
// Build + run (from this directory):
//   ./run_kr_calendar_valuestring_adopt_test.sh
//   ./run_kr_calendar_valuestring_adopt_test.sh asan

#include "KRCJsonStringAdopt.h"
#include "thirdparty/cJSON/cJSON.h"

#include <cstdio>
#include <string>

using kuikly::module::AdoptCJsonItemValuestring;
using kuikly::module::AdoptCJsonStringValue;
using kuikly::module::AdoptCalendarObjectString;
using kuikly::module::CJsonItemValuestring;

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

int main() {
    // leftover: {"opt":1} → valuestring null → helper returns "" not crash
    {
        cJSON *opObject = cJSON_Parse(R"({"opt":1})");
        expect_true("Parse({\"opt\":1}) non-null", opObject != nullptr);
        cJSON *optPtr = cJSON_GetObjectItemCaseSensitive(opObject, "opt");
        expect_true("opt item exists", optPtr != nullptr);
        expect_true("leftover opt valuestring is null", optPtr != nullptr && optPtr->valuestring == nullptr);
        expect_true("CJsonItemValuestring(number) is null", CJsonItemValuestring(optPtr) == nullptr);
        expect_eq("AdoptCJsonStringValue(null from number)", AdoptCJsonStringValue(optPtr->valuestring), "");
        expect_eq("AdoptCJsonItemValuestring({\"opt\":1})", AdoptCJsonItemValuestring(optPtr), "");
        expect_eq("AdoptCalendarObjectString opt number", AdoptCalendarObjectString(opObject, "opt"), "");
        cJSON_Delete(opObject);
    }

    // leftover: Format with {"timeMillis":0,"format":1} does not construct
    // std::string from null
    {
        cJSON *paramObj = cJSON_Parse(R"({"timeMillis":0,"format":1})");
        expect_true("Parse Format leftover JSON non-null", paramObj != nullptr);
        cJSON *formatPtr = cJSON_GetObjectItemCaseSensitive(paramObj, "format");
        expect_true("format item exists", formatPtr != nullptr);
        expect_true("leftover format valuestring is null", formatPtr != nullptr && formatPtr->valuestring == nullptr);
        expect_eq("Format leftover AdoptCJsonStringValue", AdoptCJsonStringValue(formatPtr->valuestring), "");
        expect_eq("Format leftover AdoptCalendarObjectString", AdoptCalendarObjectString(paramObj, "format"), "");
        cJSON_Delete(paramObj);
    }

    // leftover: Parse with numeric formattedTime does not crash
    {
        cJSON *paramObj = cJSON_Parse(R"({"formattedTime":1,"format":1})");
        expect_true("Parse leftover JSON non-null", paramObj != nullptr);
        cJSON *dateStrPtr = cJSON_GetObjectItemCaseSensitive(paramObj, "formattedTime");
        expect_true("formattedTime item exists", dateStrPtr != nullptr);
        expect_true("leftover formattedTime valuestring is null",
                    dateStrPtr != nullptr && dateStrPtr->valuestring == nullptr);
        expect_eq("Parse leftover formattedTime adopt", AdoptCJsonItemValuestring(dateStrPtr), "");
        expect_eq("Parse leftover format adopt", AdoptCalendarObjectString(paramObj, "format"), "");
        expect_eq("Parse leftover formattedTime object adopt",
                  AdoptCalendarObjectString(paramObj, "formattedTime"), "");
        cJSON_Delete(paramObj);
    }

    // leftover: CalDate with bad JSON "{" does not GetObjectItem on null
    {
        cJSON *opObject = cJSON_Parse("{");
        expect_true("CalDate Parse(\"{\") is nullptr", opObject == nullptr);
        // Call-site contract: skip GetObjectItem when parse failed.
        expect_eq("CalDate leftover AdoptCalendarObjectString(null)", AdoptCalendarObjectString(opObject, "opt"), "");
        expect_eq("AdoptCJsonStringValue(nullptr) default empty", AdoptCJsonStringValue(nullptr), "");
        if (opObject != nullptr) {
            std::fprintf(stderr, "FAIL CalDate must not GetObjectItem on null\n");
            ++g_failed;
            cJSON_Delete(opObject);
        } else {
            std::printf("PASS CalDate does not GetObjectItem on null\n");
        }
    }

    // String items still adopt (IsString + valuestring).
    {
        cJSON *opObject = cJSON_Parse(R"({"opt":"set"})");
        expect_eq("AdoptCalendarObjectString opt set", AdoptCalendarObjectString(opObject, "opt"), "set");
        expect_eq("AdoptCJsonStringValue(set)",
                  AdoptCJsonStringValue(CJsonItemValuestring(cJSON_GetObjectItemCaseSensitive(opObject, "opt"))),
                  "set");
        cJSON_Delete(opObject);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
