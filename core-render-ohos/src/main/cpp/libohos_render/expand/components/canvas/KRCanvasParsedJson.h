/*
 * Tencent is pleased to support the open source community by making KuiklyUI
 * available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 * Licensed under the License of KuiklyUI;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://github.com/Tencent-TDS/KuiklyUI/blob/main/LICENSE
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CORE_RENDER_OHOS_KRCANVASPARSEDJSON_H
#define CORE_RENDER_OHOS_KRCANVASPARSEDJSON_H

#include <memory>
#include <string>
#include <utility>

#include "libohos_render/utils/KRJSONObject.h"

namespace kuikly {
namespace util {

// Leftover Parse-null helper. JSONObject::Parse returns nullptr on bad JSON
// ("{", "", "not-json"). Canvas ops must adopt through this helper and
// no-op when null — never deref GetString/GetNumber on a failed parse.
// Header-only so host tests compile without Harmony drawing APIs.
inline std::shared_ptr<JSONObject> AdoptParsedJson(const std::string &params) {
    return JSONObject::Parse(params);
}

// Apply fn only when parse succeeds. Leftover unguarded deref would crash.
template <typename Fn>
inline bool AdoptParsedJson(const std::string &params, Fn &&fn) {
    auto obj = JSONObject::Parse(params);
    if (!obj) {
        return false;
    }
    std::forward<Fn>(fn)(obj);
    return true;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRCANVASPARSEDJSON_H
