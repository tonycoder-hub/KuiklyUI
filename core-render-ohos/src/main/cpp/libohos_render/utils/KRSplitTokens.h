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

#ifndef CORE_RENDER_OHOS_KRSPLITTOKENS_H
#define CORE_RENDER_OHOS_KRSPLITTOKENS_H

#include <string>
#include <vector>

#include "libohos_render/foundation/KRBorderRadiuses.h"

namespace kuikly {
namespace util {

// Leftover ConvertSplit / SplitString callers indexed [0]..[3] without a size
// check. Short CSS-like props ("8", "", "10 20", "1 2 3") produced <4 tokens
// and OOB. Strategy (documented for the leftover fix):
//   * pad-to-4 with "0" for border radiuses / content inset so a short list
//     degrades to defined zeros instead of crashing;
//   * skip update when boxShadow / textShadow has <4 tokens, and when border
//     has <3 tokens (width style color — kotlin Border.toString / iOS
//     CSSBorder). That matches sibling skip-if-short patterns
//     (KRTransformParser, KRImageView capInsets, iOS CSSBoxShadow).
// Header-only so host tests compile without ArkUI / Harmony headers.

inline std::vector<std::string> SplitByDelimiters(const std::string &str, const std::string &delimiters) {
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

inline void PadTokensTo(std::vector<std::string> &tokens, std::size_t n, const std::string &fill = "0") {
    while (tokens.size() < n) {
        tokens.push_back(fill);
    }
}

inline float TokenToFloat(const std::string &string) {
    if (string.length() == 1 && string == "0") {
        return 0;
    }
    try {
        return std::stof(string);
    } catch (...) {
        return 0;
    }
}

// leftover: "8" / "" must not index past splits.size(). Missing corners stay 0.
inline KRBorderRadiuses ConverToBorderRadiuses(const std::string &borderRadiusString) {
    auto splits = SplitByDelimiters(borderRadiusString, ",");
    PadTokensTo(splits, 4, "0");
    return KRBorderRadiuses(TokenToFloat(splits[0]), TokenToFloat(splits[1]), TokenToFloat(splits[2]),
                            TokenToFloat(splits[3]));
}

struct ContentInsetParts {
    float top = 0;
    float start = 0;
    float bottom = 0;
    float end = 0;
    bool animate = false;
};

// leftover: "10 20" / "" pad missing top/start/bottom/end with 0.
// Optional 5th token is animate (kotlin sends 0/1).
inline ContentInsetParts ParseContentInsetParts(const std::string &value) {
    auto splits = SplitByDelimiters(value, " ");
    PadTokensTo(splits, 4, "0");
    ContentInsetParts parts;
    parts.top = TokenToFloat(splits[0]);
    parts.start = TokenToFloat(splits[1]);
    parts.bottom = TokenToFloat(splits[2]);
    parts.end = TokenToFloat(splits[3]);
    if (splits.size() >= 5) {
        parts.animate = TokenToFloat(splits[4]) != 0;
    }
    return parts;
}

struct BoxShadowParts {
    float x = 0;
    float y = 0;
    float radius = 0;
    std::string color;
    bool fill = true;
};

// leftover: short boxShadow ("1 2 3", "") has <4 tokens — skip, do not index.
inline bool TryParseBoxShadowParts(const std::string &css_box_shadow, BoxShadowParts &out) {
    auto splits = SplitByDelimiters(css_box_shadow, " ");
    if (splits.size() < 4) {
        return false;
    }
    out.x = TokenToFloat(splits[0]);
    out.y = TokenToFloat(splits[1]);
    out.radius = TokenToFloat(splits[2]);
    out.color = splits[3];
    out.fill = !(splits.size() > 4 && splits[4] == "0");
    return true;
}

struct BorderParts {
    float width = 0;
    std::string style;
    std::string color;
};

// leftover: border is "width style color" (3 tokens). Skip if shorter.
// Not pad-to-4: kotlin Border.toString and the reset path ("0 solid 0") are 3
// tokens; requiring 4 would drop valid updates.
inline bool TryParseBorderParts(const std::string &borderStr, BorderParts &out) {
    auto splits = SplitByDelimiters(borderStr, " ");
    if (splits.size() < 3) {
        return false;
    }
    out.width = TokenToFloat(splits[0]);
    out.style = splits[1];
    out.color = splits[2];
    return true;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRSPLITTOKENS_H
