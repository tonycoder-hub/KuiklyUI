// Host unit test for leftover canvas JSONObject::Parse-null deref.
//
// JSONObject::Parse returns nullptr on bad JSON. Leftover canvas ops
// (SetLineCap / MoveTo / DrawText / Transform / ...) deref'd that pointer
// for GetString/GetNumber. Siblings SetStrokeStyle / SetFillStyle / Arc
// already guard if (paramObj). AdoptParsedJson no-ops on nullptr so host
// tests compile without Harmony drawing APIs.
//
// Build + run (from this directory):
//   ./run_kr_canvas_parsed_json_test.sh
//   ./run_kr_canvas_parsed_json_test.sh asan

#include "KRCanvasParsedJson.h"

#include <cstdio>
#include <string>

using kuikly::util::AdoptParsedJson;
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

int main() {
    // leftover: Parse("{") / Parse("") / Parse("not-json") is nullptr.
    // A leftover unguarded deref (obj->GetString / obj->GetNumber) would crash.
    // Helper must no-op.
    {
        expect_true("Parse(\"{\") is nullptr", JSONObject::Parse("{") == nullptr);
        expect_true("Parse(\"\") is nullptr", JSONObject::Parse("") == nullptr);
        expect_true("Parse(\"not-json\") is nullptr", JSONObject::Parse("not-json") == nullptr);

        expect_true("AdoptParsedJson(\"{\") is nullptr", AdoptParsedJson("{") == nullptr);
        expect_true("AdoptParsedJson(\"\") is nullptr", AdoptParsedJson("") == nullptr);
        expect_true("AdoptParsedJson(\"not-json\") is nullptr", AdoptParsedJson("not-json") == nullptr);

        bool invoked_brace = false;
        bool invoked_empty = false;
        bool invoked_not_json = false;
        expect_true("AdoptParsedJson(\"{\") no-op",
                    !AdoptParsedJson("{", [&](auto &) { invoked_brace = true; }) && !invoked_brace);
        expect_true("AdoptParsedJson(\"\") no-op",
                    !AdoptParsedJson("", [&](auto &) { invoked_empty = true; }) && !invoked_empty);
        expect_true("AdoptParsedJson(\"not-json\") no-op",
                    !AdoptParsedJson("not-json", [&](auto &) { invoked_not_json = true; }) &&
                        !invoked_not_json);

        // Call-site contract: only GetString/GetNumber after a non-null adopt.
        auto bad = AdoptParsedJson("{");
        if (bad) {
            // leftover unguarded: bad->GetString("style") would crash
            std::fprintf(stderr, "FAIL leftover unguarded deref skipped\n");
            ++g_failed;
        } else {
            std::printf("PASS leftover unguarded deref skipped\n");
        }
    }

    // leftover: valid {"style":"round"} still adopts
    {
        auto obj = AdoptParsedJson(R"({"style":"round"})");
        expect_true("AdoptParsedJson({\"style\":\"round\"}) non-null", obj != nullptr);
        expect_eq("GetString style", obj ? obj->GetString("style") : "", "round");

        std::string adopted_style;
        bool adopted = AdoptParsedJson(R"({"style":"round"})", [&](auto parsed) {
            adopted_style = parsed->GetString("style");
        });
        expect_true("AdoptParsedJson({\"style\":\"round\"}) applies", adopted);
        expect_eq("adopted style", adopted_style, "round");
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
