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

#ifndef CORE_RENDER_OHOS_KRCANVASNUMERICPARSE_H
#define CORE_RENDER_OHOS_KRCANVASNUMERICPARSE_H

#include <string>
#include <vector>

namespace kuikly {
namespace util {

// Leftover canvas SetFont / processColorStops helpers. Bare std::stoi /
// std::stof throw std::invalid_argument on empty or non-numeric tokens
// ("", "bold", location "abc" from "#f00 abc"). Header-only so host g++
// can compile without Harmony drawing APIs.

inline bool ParseCanvasFontWeight(const std::string &weight_str, int &out) {
    if (weight_str.empty()) {
        return false;
    }
    try {
        out = std::stoi(weight_str);
        return true;
    } catch (...) {
        return false;
    }
}

inline bool ParseCanvasColorStopLocation(const std::string &location_str, float &out) {
    if (location_str.empty()) {
        return false;
    }
    try {
        out = std::stof(location_str);
        return true;
    } catch (...) {
        return false;
    }
}

// Same split as ConvertSplit (KRConvertUtil.cpp). Host tests cannot compile
// that translation unit (Harmony headers).
inline std::vector<std::string> SplitCanvasTokens(const std::string &str, const std::string &delimiters) {
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

// Parse leftover canvas "color location,..." list. On a bad location token,
// skip that stop (do not throw; do not push a mismatched color).
inline void ProcessCanvasColorStops(const std::string &color_stops_str, std::vector<std::string> &colors,
                                    std::vector<float> &locations) {
    const std::vector<std::string> splits = SplitCanvasTokens(color_stops_str, ",");
    for (const auto &color_stop_str : splits) {
        if (color_stop_str.empty()) {
            continue;
        }
        const std::vector<std::string> color_and_stop = SplitCanvasTokens(color_stop_str, " ");
        if (color_and_stop.size() < 2) {
            continue;
        }
        float location = 0.f;
        if (!ParseCanvasColorStopLocation(color_and_stop[1], location)) {
            continue;
        }
        colors.push_back(color_and_stop[0]);
        locations.push_back(location);
    }
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRCANVASNUMERICPARSE_H
