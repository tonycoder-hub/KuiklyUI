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

#ifndef CORE_RENDER_OHOS_KRCONTENTOFFSETPARSE_H
#define CORE_RENDER_OHOS_KRCONTENTOFFSETPARSE_H

#include <sstream>
#include <string>
#include <vector>

namespace kuikly {
namespace util {

// Leftover KRScrollerView::SetContentOffset helpers.
// SplitString(value, ' ') then indexed [0]/[1]/[2] with no size check.
// iOS css_contentOffsetWithParams uses:
//   animated = count > 2 ? points[2] : NO
//   firstObject / points[1] for x/y
// so "10 20" (no animate token) is valid on iOS and crashes on OHOS.
// #1663 guarded inset / border / shadow short props, not this method.
// Header-only so host g++ can compile without Harmony / ArkUI.

inline std::vector<std::string> SplitContentOffsetTokens(const std::string &str) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

inline float ParseContentOffsetFloat(const std::string &token) {
    try {
        return std::stof(token);
    } catch (...) {
        return 0.f;
    }
}

inline int ParseContentOffsetInt(const std::string &token) {
    try {
        return std::stoi(token);
    } catch (...) {
        return 0;
    }
}

inline bool ParseContentOffsetBool(const std::string &token) {
    if (token.empty() || token == "0" || token == "false") {
        return false;
    }
    try {
        return std::stof(token) != 0.f;
    } catch (...) {
        return true;
    }
}

struct KRContentOffsetArgs {
    float offset_x = 0.f;
    float offset_y = 0.f;
    bool animate = false;
    int duration = 0;
    float damping = 0.f;
    int curve = 0;
};

// Policy:
//   size < 2 → return false (do not write outs; cannot form a point)
//   size == 2 → x, y, animate=false (iOS sibling)
//   size >= 3 → x, y, animate, plus existing optional duration/damping/curve
inline bool ParseContentOffset(const std::string &value, KRContentOffsetArgs &out) {
    const std::vector<std::string> tokens = SplitContentOffsetTokens(value);
    if (tokens.size() < 2) {
        return false;
    }
    out.offset_x = ParseContentOffsetFloat(tokens[0]);
    out.offset_y = ParseContentOffsetFloat(tokens[1]);
    out.animate = tokens.size() > 2 ? ParseContentOffsetBool(tokens[2]) : false;
    out.duration = tokens.size() > 3 ? ParseContentOffsetInt(tokens[3]) : 0;
    out.damping = tokens.size() > 4 ? ParseContentOffsetFloat(tokens[4]) : 0.f;
    out.curve = tokens.size() > 6 ? ParseContentOffsetInt(tokens[6]) : 0;
    return true;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRCONTENTOFFSETPARSE_H
