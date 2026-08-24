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

#ifndef CORE_RENDER_OHOS_KRSELECTEDCONTENTCLAMP_H
#define CORE_RENDER_OHOS_KRSELECTEDCONTENTCLAMP_H

#include <cstddef>
#include <string>
#include <utility>

namespace kuikly {
namespace util {

// Leftover GetSelectedContent only clamped `end`. `start > size` throws
// std::out_of_range from substr; `start > end` underflows size_t on
// (sel_end - start). Header-only so host g++ can compile without ArkUI.

inline size_t ClampUtf16Index(int index, size_t size) {
    if (index <= 0) {
        return 0;
    }
    const size_t value = static_cast<size_t>(index);
    return value > size ? size : value;
}

// Clamp start/end into [0, size] and force start <= end so later
// substr(start, end - start) and substr(0, start) cannot throw or wrap.
inline std::pair<size_t, size_t> ClampSelectedUtf16Range(int start, int end, size_t size) {
    size_t sel_start = ClampUtf16Index(start, size);
    size_t sel_end = ClampUtf16Index(end, size);
    if (sel_start > sel_end) {
        sel_start = sel_end;
    }
    return {sel_start, sel_end};
}

struct KRSelectedContentParts {
    std::u16string pre;
    std::u16string selected;
    std::u16string post;
};

inline KRSelectedContentParts SliceSelectedUtf16Content(const std::u16string &str16, int start, int end) {
    const auto [sel_start, sel_end] = ClampSelectedUtf16Range(start, end, str16.size());

    KRSelectedContentParts parts;
    if (sel_start > 0) {
        parts.pre = str16.substr(0, sel_start);
    }
    parts.selected = str16.substr(sel_start, sel_end - sel_start);
    if (sel_end < str16.size()) {
        parts.post = str16.substr(sel_end);
    }
    return parts;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRSELECTEDCONTENTCLAMP_H
