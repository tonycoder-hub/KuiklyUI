#ifndef CORE_RENDER_OHOS_TEST_RENDER_CONTEXT_PARAMS_HOST_H
#define CORE_RENDER_OHOS_TEST_RENDER_CONTEXT_PARAMS_HOST_H

#include <memory>
#include <string>
#include <unordered_map>

// Extracted leftover KRRenderContextParams ctor / PageParam map[] + end()-deref.
//
// Leftover:
//   page_data_map["executeMode"]->toInt() / ["contextCode"]->toString()
//   PageParam: find("param")->second with no end() check
//   page_data_ used without a null check before toMap()
//
// Production (no Harmony):
//   find() + null check
//   missing executeMode → default-mode branch (no deref)
//   missing contextCode → empty string
//   PageParam missing/end → nullptr
//   !page_data_ → skip toMap()
//
// Header-only so host leftover tests compile without Harmony.

struct HostValue {
    int n = 0;
    std::string s;
    int toInt() const { return n; }
    std::string toString() const { return s; }
};

using HostAny = std::shared_ptr<HostValue>;
using HostMap = std::unordered_map<std::string, HostAny>;

struct ContextHost {
    HostAny page_data;
    bool used_default_mode = false;
    int execute_mode = -1;
    std::string context_code;
    bool deref_attempted = false;

    HostMap toMap() const {
        if (!page_data) {
            return HostMap{};
        }
        // Host page_data itself stores the map in s as unused; tests pass a HostMap separately.
        return HostMap{};
    }
};

struct CtorResult {
    bool used_default_mode = false;
    int execute_mode = -1;
    std::string context_code;
};

inline CtorResult InitFromPageData(const HostAny &page_data, const HostMap &page_data_map) {
    CtorResult r;
    if (!page_data) {
        r.used_default_mode = true;
        r.context_code = "";
        return r;
    }
    auto mode_it = page_data_map.find("executeMode");
    if (mode_it != page_data_map.end() && mode_it->second) {
        r.execute_mode = mode_it->second->toInt();
        r.used_default_mode = false;
    } else {
        r.used_default_mode = true;
    }
    auto code_it = page_data_map.find("contextCode");
    if (code_it != page_data_map.end() && code_it->second) {
        r.context_code = code_it->second->toString();
    } else {
        r.context_code = "";
    }
    return r;
}

inline HostAny PageParam(const HostAny &page_data, const HostMap &page_data_map) {
    if (!page_data) {
        return nullptr;
    }
    auto it = page_data_map.find("param");
    if (it == page_data_map.end()) {
        return nullptr;
    }
    return it->second;
}

#endif  // CORE_RENDER_OHOS_TEST_RENDER_CONTEXT_PARAMS_HOST_H
