// Host unit test for leftover KRMemoryCacheModule SetObject/CacheImage map[].
//
// Leftover:
//   auto map = params->toMap();
//   auto key = map[kParamNameKey]->toString();
//   auto value = map[kParamNameValue];
//   auto src = map[kParamNameSrc]->toString();
//
// operator[] default-inserts nullptr shared_ptr when key/src/value is
// missing. ->toString() is a null deref / crash. Get() already uses find.
//
// Fix: find + null-check like Get(); missing key/src/value → empty / error,
// no deref. Valid {key,value} / {src} still work.
//
// Harmony image APIs block host compile of KRMemoryCacheModule.cpp.
// Header-only helper kr_memory_cache_map_find.h mirrors the FIXED lookup.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_memory_cache_map_find_test.sh
//   ./run_kr_memory_cache_map_find_test.sh asan

#include "kr_memory_cache_map_find.h"

#include <cstdio>
#include <string>

using leftover_memory_cache::CacheImage;
using leftover_memory_cache::HostMap;
using leftover_memory_cache::MakeHostValue;
using leftover_memory_cache::SetObject;
using leftover_memory_cache::kParamNameKey;
using leftover_memory_cache::kParamNameSrc;
using leftover_memory_cache::kParamNameValue;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void leftover_operator_index_inserts_nullptr() {
    HostMap map;
    auto inserted = map[kParamNameKey];
    expect_true("leftover map[] missing key inserts default shared_ptr", !inserted);
    expect_true("leftover map[] grew the map", map.find(kParamNameKey) != map.end());
    expect_true("leftover map[] stored nullptr", map.find(kParamNameKey)->second == nullptr);
}

int main() {
    leftover_operator_index_inserts_nullptr();

    // SetObject({}) must not crash; return empty; nothing stored.
    {
        auto r = SetObject(HostMap{});
        expect_true("SetObject({}) empty return", r.empty_return);
        expect_true("SetObject({}) not stored", !r.stored);
        expect_true("SetObject({}) key empty", r.key.empty());
    }

    // CacheImage({}) must not crash; return error.
    {
        auto r = CacheImage(HostMap{});
        expect_true("CacheImage({}) is error", r.is_error);
        expect_true("CacheImage({}) errorCode -1", r.error_code == -1);
        expect_true("CacheImage({}) errorMsg missing src", r.error_msg == "missing src");
        expect_true("CacheImage({}) src empty", r.src.empty());
    }

    // missing value only
    {
        HostMap map;
        map[kParamNameKey] = MakeHostValue("k");
        auto r = SetObject(map);
        expect_true("SetObject({key}) empty return", r.empty_return);
        expect_true("SetObject({key}) not stored", !r.stored);
    }

    // missing key only
    {
        HostMap map;
        map[kParamNameValue] = MakeHostValue("v");
        auto r = SetObject(map);
        expect_true("SetObject({value}) empty return", r.empty_return);
        expect_true("SetObject({value}) not stored", !r.stored);
    }

    // explicit nullptr shared_ptr at key / src (operator[] leftover payload)
    {
        HostMap map;
        map[kParamNameKey] = nullptr;
        map[kParamNameValue] = MakeHostValue("v");
        auto r = SetObject(map);
        expect_true("SetObject(null key) not stored", !r.stored);
    }
    {
        HostMap map;
        map[kParamNameSrc] = nullptr;
        auto r = CacheImage(map);
        expect_true("CacheImage(null src) is error", r.is_error);
    }

    // valid {key,value} still works
    {
        HostMap map;
        map[kParamNameKey] = MakeHostValue("cache-a");
        map[kParamNameValue] = MakeHostValue("payload");
        auto r = SetObject(map);
        expect_true("SetObject({key,value}) stored", r.stored);
        expect_true("SetObject({key,value}) key", r.key == "cache-a");
        expect_true("SetObject({key,value}) value", r.value == "payload");
    }

    // valid {src} still works
    {
        HostMap map;
        map[kParamNameSrc] = MakeHostValue("https://example.com/a.png");
        auto r = CacheImage(map);
        expect_true("CacheImage({src}) not error", !r.is_error);
        expect_true("CacheImage({src}) src", r.src == "https://example.com/a.png");
        expect_true("CacheImage({src}) errorCode 0", r.error_code == 0);
    }

    // empty-string src is present (valid {src}), not missing
    {
        HostMap map;
        map[kParamNameSrc] = MakeHostValue("");
        auto r = CacheImage(map);
        expect_true("CacheImage({src:\"\"}) not error", !r.is_error);
        expect_true("CacheImage({src:\"\"}) src empty string", r.src.empty());
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover memory-cache map-find tests passed\n");
    return 0;
}
