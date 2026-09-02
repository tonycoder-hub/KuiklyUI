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

#ifndef CORE_RENDER_OHOS_KRCJSONSTRINGADOPT_H
#define CORE_RENDER_OHOS_KRCJSONSTRINGADOPT_H

#include <string>

#include "thirdparty/cJSON/cJSON.h"

namespace kuikly {
namespace module {

// Leftover adopt helper (host-testable, NDK-free). Same contract as #1651
// AdoptCJsonStringValue: cJSON sets valuestring only for string items. A
// number/bool/null field has valuestring == NULL. std::string(NULL) is UB —
// never construct std::string from a null C string.
inline std::string AdoptCJsonStringValue(const char *value, const std::string &default_value = "") {
    if (value == nullptr) {
        return default_value;
    }
    return std::string(value);
}

// Require cJSON_IsString before reading valuestring. cJSON_IsString(NULL) is
// false, so a missing item also defaults.
inline const char *CJsonItemValuestring(const cJSON *item) {
    if (!cJSON_IsString(item)) {
        return nullptr;
    }
    return item->valuestring;
}

inline std::string AdoptCJsonItemValuestring(const cJSON *item, const std::string &default_value = "") {
    return AdoptCJsonStringValue(CJsonItemValuestring(item), default_value);
}

// leftover CalDate/Format/Parse: skip GetObjectItem when Parse failed
// (opObject / paramObj is nullptr).
inline std::string AdoptCalendarObjectString(cJSON *object, const char *key,
                                            const std::string &default_value = "") {
    if (object == nullptr || key == nullptr) {
        return default_value;
    }
    return AdoptCJsonItemValuestring(cJSON_GetObjectItemCaseSensitive(object, key), default_value);
}

}  // namespace module
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRCJSONSTRINGADOPT_H
