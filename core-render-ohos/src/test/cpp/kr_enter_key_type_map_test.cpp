// Host unit test for leftover returnKeyType mapping.
//
// KREnterKeyTypeMap.h has no Harmony SDK dependency. Host g++/clang++ is enough.
//
// Build + run (from this directory):
//   ./run_kr_enter_key_type_map_test.sh
//   ./run_kr_enter_key_type_map_test.sh asan

#include "libohos_render/utils/KREnterKeyTypeMap.h"

#include <cstdio>
#include <string>

using kuikly::util::EnterKeyTypeCodeToName;
using kuikly::util::kEnterKeyTypeDone;
using kuikly::util::kEnterKeyTypeGo;
using kuikly::util::kEnterKeyTypeNewLine;
using kuikly::util::kEnterKeyTypeNext;
using kuikly::util::kEnterKeyTypePrevious;
using kuikly::util::kEnterKeyTypeSearch;
using kuikly::util::kEnterKeyTypeSend;
using kuikly::util::MapEnterKeyTypeName;

static int g_failed = 0;

static void expect_eq_int(const char *name, int got, int want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq_str(const char *name, const char *got, const char *want) {
    if (std::string(got) != want) {
        std::fprintf(stderr, "FAIL %s: got \"%s\" want \"%s\"\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    // Leftover bug: "none" must not map to NEW_LINE (IME shows "New Line"
    // and can insert newlines on single-line fields). Android maps "none"
    // → IME_ACTION_NONE; ArkUI has no NONE, so fall through to DONE.
    expect_true("none != NEW_LINE", MapEnterKeyTypeName("none") != kEnterKeyTypeNewLine);
    expect_eq_int("none == DONE", MapEnterKeyTypeName("none"), kEnterKeyTypeDone);

    // Explicit newline aliases only.
    expect_eq_int("newline == NEW_LINE", MapEnterKeyTypeName("newline"), kEnterKeyTypeNewLine);
    expect_eq_int("newLine == NEW_LINE", MapEnterKeyTypeName("newLine"), kEnterKeyTypeNewLine);

    // Known returnKeyType strings keep their ArkUI counterparts.
    expect_eq_int("search", MapEnterKeyTypeName("search"), kEnterKeyTypeSearch);
    expect_eq_int("send", MapEnterKeyTypeName("send"), kEnterKeyTypeSend);
    expect_eq_int("go", MapEnterKeyTypeName("go"), kEnterKeyTypeGo);
    expect_eq_int("done", MapEnterKeyTypeName("done"), kEnterKeyTypeDone);
    expect_eq_int("next", MapEnterKeyTypeName("next"), kEnterKeyTypeNext);
    expect_eq_int("previous", MapEnterKeyTypeName("previous"), kEnterKeyTypePrevious);

    // Unknown / empty also fall through to DONE (ArkUI default).
    expect_eq_int("empty == DONE", MapEnterKeyTypeName(""), kEnterKeyTypeDone);
    expect_eq_int("unknown == DONE", MapEnterKeyTypeName("continue"), kEnterKeyTypeDone);

    // Reverse map: NEW_LINE must not be empty (was default → "").
    expect_true("NEW_LINE reverse not empty", EnterKeyTypeCodeToName(kEnterKeyTypeNewLine)[0] != '\0');
    expect_eq_str("NEW_LINE → newline", EnterKeyTypeCodeToName(kEnterKeyTypeNewLine), "newline");
    expect_eq_str("DONE → done", EnterKeyTypeCodeToName(kEnterKeyTypeDone), "done");
    expect_eq_str("SEARCH → search", EnterKeyTypeCodeToName(kEnterKeyTypeSearch), "search");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
