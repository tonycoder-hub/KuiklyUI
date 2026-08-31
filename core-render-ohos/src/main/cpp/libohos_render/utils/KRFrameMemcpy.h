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

#ifndef CORE_RENDER_OHOS_KRFRAMEMEMCPY_H
#define CORE_RENDER_OHOS_KRFRAMEMEMCPY_H

#include <cstring>
#include <string_view>

namespace kuikly {
namespace util {

// Wire size of a kFrame binary payload: x, y, width, height. KRRect also has
// a trailing isDefault_ flag, so sizeof(KRRect) is larger than this.
inline constexpr std::size_t kKRFrameFloatBytes = sizeof(float) * 4;

// leftover: KRBasePropsHandler kFrame (and sibling memcpy sites) copied
// caller-controlled s.size() into a stack KRRect. Size != 16 is reject
// (empty / text / oversized smash). On accept, copy only the 4 floats so
// isDefault_ is not clobbered. Header-only so host tests compile without ArkUI.
template <typename RectT>
inline bool TryCopyFrameFloats(std::string_view payload, RectT &out) {
    if (payload.size() != kKRFrameFloatBytes) {
        return false;
    }
    float xywh[4];
    memcpy(xywh, payload.data(), kKRFrameFloatBytes);
    out.x = xywh[0];
    out.y = xywh[1];
    out.width = xywh[2];
    out.height = xywh[3];
    return true;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRFRAMEMEMCPY_H
