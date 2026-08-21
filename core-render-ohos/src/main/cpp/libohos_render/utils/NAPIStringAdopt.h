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

#ifndef CORE_RENDER_OHOS_NAPISTRINGADOPT_H
#define CORE_RENDER_OHOS_NAPISTRINGADOPT_H

#include <string>

namespace kuikly {
namespace util {

// Copy a leftover NAPI UTF-8 C string into std::string without constructing
// from nullptr. getNApiArgsString returns 0 after napi_throw_error; leftover
// getNApiArgsStdString did std::string(resStr), which is UB on null.
// The helper only copies; the caller still owns and may free resStr.
inline std::string adopt_napi_cstr(const char *resStr) {
    if (resStr == nullptr) {
        return "";
    }
    return std::string(resStr);
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_NAPISTRINGADOPT_H
