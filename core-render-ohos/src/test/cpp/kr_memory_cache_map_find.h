#ifndef CORE_RENDER_OHOS_TEST_KR_MEMORY_CACHE_MAP_FIND_H
#define CORE_RENDER_OHOS_TEST_KR_MEMORY_CACHE_MAP_FIND_H

#include <memory>
#include <string>
#include <unordered_map>

// Host leftover helper for KRMemoryCacheModule SetObject / CacheImage.
//
// Production leftover:
//   auto map = params->toMap();
//   auto key = map[kParamNameKey]->toString();   // SetObject
//   auto value = map[kParamNameValue];
//   auto src = map[kParamNameSrc]->toString();   // CacheImage
//
// Map is unordered_map<string, shared_ptr<...>>. operator[] default-inserts
// a nullptr shared_ptr when the key is missing; ->toString() is a null deref.
// Get() already uses find (no operator[]).
//
// Production cannot host-compile (Harmony image APIs). This header mirrors
// the FIXED lookup: find + null-check. Missing key/src/value → empty / error,
// no deref. Valid {key,value} / {src} still work.
//
// Header-only so leftover host tests compile without Harmony.

namespace leftover_memory_cache {

constexpr char kParamNameKey[] = "key";
constexpr char kParamNameValue[] = "value";
constexpr char kParamNameSrc[] = "src";

struct HostValue {
    std::string str;
    std::string toString() const { return str; }
};

using HostAny = std::shared_ptr<HostValue>;
using HostMap = std::unordered_map<std::string, HostAny>;

inline HostAny MakeHostValue(const std::string &s) {
    return std::make_shared<HostValue>(HostValue{s});
}

// find + null-check, same shape as Get() plus a shared_ptr guard.
template <typename Map>
inline typename Map::mapped_type FindMapValue(const Map &map, const char *key) {
    auto it = map.find(key);
    if (it == map.end() || !it->second) {
        return typename Map::mapped_type{};
    }
    return it->second;
}

struct SetObjectResult {
    bool empty_return = true;
    bool stored = false;
    std::string key;
    std::string value;
};

// SetObject({}): missing key or value → empty, no deref, nothing stored.
// Valid {key,value} still stores.
inline SetObjectResult SetObject(const HostMap &map) {
    auto key_v = FindMapValue(map, kParamNameKey);
    auto value_v = FindMapValue(map, kParamNameValue);
    if (!key_v || !value_v) {
        return SetObjectResult{};
    }
    SetObjectResult r;
    r.empty_return = true;  // production success also returns KREmptyValue()
    r.stored = true;
    r.key = key_v->toString();
    r.value = value_v->toString();
    return r;
}

struct CacheImageResult {
    bool is_error = false;
    int error_code = 0;
    std::string error_msg;
    std::string src;
};

// CacheImage({}): missing / null src → GenerateError, no deref.
// Valid {src} still extracts src (Harmony load is not host-compiled).
inline CacheImageResult CacheImage(const HostMap &map) {
    auto src_v = FindMapValue(map, kParamNameSrc);
    if (!src_v) {
        CacheImageResult r;
        r.is_error = true;
        r.error_code = -1;
        r.error_msg = "missing src";
        return r;
    }
    CacheImageResult r;
    r.src = src_v->toString();
    return r;
}

}  // namespace leftover_memory_cache

#endif  // CORE_RENDER_OHOS_TEST_KR_MEMORY_CACHE_MAP_FIND_H
