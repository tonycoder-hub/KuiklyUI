// Host leftover helper: KRRenderValue::toMap() string path without Harmony.
//
// Production KRRenderValue.h pulls ark_runtime / NAPI / JSVM and cannot
// host-compile. This header mirrors the FIXED string path using real cJSON:
//   cJSON_Parse
//   empty only if parse-null
//   if (!cJSON_IsObject(cjson)) { cJSON_Delete(cjson); return Map(); }
//   skip children with item->string == nullptr
//   map[item->string] = ...
//
// Leftover: array JSON children have item->string == nullptr.
// Map is unordered_map<std::string, ...> so map[item->string] is
// std::string(nullptr) UB; libstdc++ throws; uncaught in render process.

#pragma once

#include "cJSON.h"

#include <string>
#include <unordered_map>

namespace kuikly {
namespace leftover_tomap {

using Map = std::unordered_map<std::string, bool>;

// Mirror of KRRenderValue::toMap() string path after the leftover fix.
inline Map toMapFromJsonString(const std::string &str) {
    cJSON *cjson = cJSON_Parse(str.c_str());
    if (cjson == nullptr) {
        return Map();
    }
    if (!cJSON_IsObject(cjson)) {
        cJSON_Delete(cjson);
        return Map();
    }
    Map map;
    for (cJSON *item = cjson->child; item != NULL; item = item->next) {
        if (item->string == nullptr) {
            continue;
        }
        map[item->string] = true;
    }
    cJSON_Delete(cjson);
    return map;
}

}  // namespace leftover_tomap
}  // namespace kuikly
