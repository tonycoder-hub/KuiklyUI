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

#ifndef CORE_RENDER_OHOS_KRCAPINSETSPARSE_H
#define CORE_RENDER_OHOS_KRCAPINSETSPARSE_H

#include <string>
#include <vector>

namespace kuikly {
namespace util {

// Leftover KRImageView::SetCapInsets helpers. ConvertSplit already checks
// items.size() >= 4 (unlike #1663 short-prop OOB), but tokens still used
// bare std::stof. Non-numeric tokens ("a", "x") throw std::invalid_argument.
// Sibling SetColorFilter already try/catches stof and stores 0 on failure.
// Header-only so host g++ can compile without Harmony / ArkUI.

inline float ParseCapInsetFloat(const std::string &token) {
    try {
        return std::stof(token);
    } catch (...) {
        return 0.f;
    }
}

// Same split as ConvertSplit (KRConvertUtil.cpp). Host tests cannot compile
// that translation unit (Harmony headers).
inline std::vector<std::string> SplitCapInsetTokens(const std::string &str, const std::string &delimiters) {
    std::vector<std::string> result;
    std::size_t start = 0;
    std::size_t end = str.find_first_of(delimiters);

    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }

    result.push_back(str.substr(start));
    return result;
}

struct KRCapInsetsValue {
    float top = 0.f;
    float left = 0.f;
    float bottom = 0.f;
    float right = 0.f;
};

// Parse leftover capInsets "top left bottom right".
// Policy: size < 4 → return false (do not write outs, same skip as today).
// size >= 4 → parse each token with try/catch; on failure that inset is 0
// (SetColorFilter sibling). Return true so the caller still applies lattice.
inline bool ParseCapInsets4(const std::string &value, KRCapInsetsValue &out) {
    const std::vector<std::string> items = SplitCapInsetTokens(value, " ");
    if (items.size() < 4) {
        return false;
    }
    out.top = ParseCapInsetFloat(items[0]);
    out.left = ParseCapInsetFloat(items[1]);
    out.bottom = ParseCapInsetFloat(items[2]);
    out.right = ParseCapInsetFloat(items[3]);
    return true;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRCAPINSETSPARSE_H
