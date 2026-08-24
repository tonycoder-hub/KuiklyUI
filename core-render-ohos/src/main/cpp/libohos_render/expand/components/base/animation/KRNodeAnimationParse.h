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

#ifndef CORE_RENDER_OHOS_KRNODEANIMATIONPARSE_H
#define CORE_RENDER_OHOS_KRNODEANIMATIONPARSE_H

#include <string>
#include <vector>

namespace kuikly {
namespace util {

// Space-split animation config indices. Same leftover layout as
// KRNodeAnimation::parseAnimation.
constexpr int kAnimationTypeIndex = 0;
constexpr int kTimingFuncTypeIndex = 1;
constexpr int kDurationIndex = 2;
constexpr int kDampingIndex = 3;
constexpr int kVelocityIndex = 4;
constexpr int kDelayIndex = 5;
constexpr int kRepeatIndex = 6;
constexpr int kAnimationKeyIndex = 7;

// Parsed fields. Defaults match KRNodeAnimation member initializers (0 / "").
struct KRNodeAnimationParsed {
    int animationType = 0;
    int timingFuncType = 0;
    float duration = 0;
    float damping = 0;
    float velocity = 0;
    float delay = 0;
    bool repeatForever = false;
    std::string animationKey;
};

// Same split as ConvertSplit (KRConvertUtil.cpp). Host tests cannot compile
// that translation unit (Harmony / ArkUI headers).
inline std::vector<std::string> SplitAnimationTokens(const std::string &str, const std::string &delimiters) {
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

inline bool ParseAnimationIntToken(const std::string &token, int &out) {
    try {
        out = std::stoi(token);
        return true;
    } catch (...) {
        return false;
    }
}

inline bool ParseAnimationFloatToken(const std::string &token, float &out) {
    try {
        out = std::stof(token);
        return true;
    } catch (...) {
        return false;
    }
}

// Leftover parseAnimation: short strings ("", "0", "1 2", "a b c") used to
// OOB-index or throw std::invalid_argument via bare stoi/stof. Require 5
// tokens before touching [0]..[4]. On a bad numeric token, keep default 0
// and skip that field. Delay / repeat / key keep their size() guards.
inline KRNodeAnimationParsed ParseNodeAnimation(const std::string &animation) {
    KRNodeAnimationParsed parsed;
    const std::vector<std::string> animationSpilt = SplitAnimationTokens(animation, " ");
    if (animationSpilt.size() <= static_cast<std::size_t>(kVelocityIndex)) {
        return parsed;
    }

    int int_value = 0;
    float float_value = 0;

    if (ParseAnimationIntToken(animationSpilt[kAnimationTypeIndex], int_value)) {
        parsed.animationType = int_value;
    }
    if (ParseAnimationIntToken(animationSpilt[kTimingFuncTypeIndex], int_value)) {
        parsed.timingFuncType = int_value;
    }
    if (ParseAnimationFloatToken(animationSpilt[kDurationIndex], float_value)) {
        parsed.duration = float_value;
    }
    if (ParseAnimationFloatToken(animationSpilt[kDampingIndex], float_value)) {
        parsed.damping = float_value;
    }
    if (ParseAnimationFloatToken(animationSpilt[kVelocityIndex], float_value)) {
        parsed.velocity = float_value;
    }

    // Compatible with older animation strings that omit trailing fields.
    if (animationSpilt.size() > static_cast<std::size_t>(kDelayIndex)) {
        if (ParseAnimationFloatToken(animationSpilt[kDelayIndex], float_value)) {
            parsed.delay = float_value;
        }
    }
    if (animationSpilt.size() > static_cast<std::size_t>(kRepeatIndex)) {
        if (ParseAnimationIntToken(animationSpilt[kRepeatIndex], int_value)) {
            parsed.repeatForever = int_value == 1;
        }
    }
    if (animationSpilt.size() > static_cast<std::size_t>(kAnimationKeyIndex)) {
        parsed.animationKey = animationSpilt[kAnimationKeyIndex];
    }
    return parsed;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRNODEANIMATIONPARSE_H
