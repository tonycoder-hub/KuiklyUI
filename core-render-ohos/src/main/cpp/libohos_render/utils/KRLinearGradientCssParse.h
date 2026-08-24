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

#ifndef CORE_RENDER_OHOS_KRLINEARGRADIENTCSSPARSE_H
#define CORE_RENDER_OHOS_KRLINEARGRADIENTCSSPARSE_H

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace kuikly {
namespace util {
namespace linear_gradient_detail {

// Header-only CSS linear-gradient parser (no ArkUI / Harmony NDK).
// Used by KRLinearGradientParser and by host g++ tests.

inline std::string TrimCssToken(const std::string &s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

inline std::vector<std::string> SplitCssTokens(const std::string &str, const std::string &delimiters) {
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

inline std::vector<std::string> SplitAndTrimNonEmpty(const std::string &str, const std::string &delimiters) {
    std::vector<std::string> result;
    const auto raw = SplitCssTokens(str, delimiters);
    for (const auto &token : raw) {
        const std::string trimmed = TrimCssToken(token);
        if (!trimmed.empty()) {
            result.push_back(trimmed);
        }
    }
    return result;
}

inline uint32_t DefaultCssGradientColor(const std::string &colorStr) {
    try {
        return static_cast<uint32_t>(std::stol(colorStr));
    } catch (...) {
        return 0;
    }
}

#ifdef CORE_RENDER_OHOS_KRCONVERTUTIL_H
inline uint32_t ConvertCssGradientColor(const std::string &colorStr) {
    return ConvertToHexColor(colorStr);
}
#else
inline uint32_t ConvertCssGradientColor(const std::string &colorStr) {
    return DefaultCssGradientColor(colorStr);
}
#endif

// Parse "linear-gradient(<dir>, <color> <stop>, ...)".
// Returns false (and does not throw) for missing trailing ')', short strings,
// empty / non-numeric direction, stof failures, or no color stops.
inline bool ParseFromCssLinearGradient(const std::string &cssGradient, int &direction, std::vector<uint32_t> &colors,
                                       std::vector<float> &locations) {
    const std::string prefix = "linear-gradient(";
    if (cssGradient.size() < prefix.size() + 1) {
        return false;
    }
    if (cssGradient.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    if (cssGradient.back() != ')') {
        return false;
    }

    const size_t inner_len = cssGradient.size() - prefix.size() - 1;
    const std::string inner = cssGradient.substr(prefix.size(), inner_len);
    const std::vector<std::string> splits = SplitCssTokens(inner, ",");
    if (splits.empty()) {
        return false;
    }

    try {
        const std::string dir_tok = TrimCssToken(splits[0]);
        if (dir_tok.empty()) {
            return false;
        }
        direction = std::stoi(dir_tok);
    } catch (...) {
        return false;
    }

    colors.clear();
    locations.clear();

    for (size_t i = 1; i < splits.size(); ++i) {
        const std::string color_stop_str = TrimCssToken(splits[i]);
        if (color_stop_str.empty()) {
            return false;
        }
        const std::vector<std::string> color_and_stop = SplitAndTrimNonEmpty(color_stop_str, " ");
        if (color_and_stop.size() < 2) {
            return false;
        }
        if (color_and_stop[0].empty() || color_and_stop[1].empty()) {
            return false;
        }
        try {
            colors.push_back(ConvertCssGradientColor(color_and_stop[0]));
            locations.push_back(std::stof(color_and_stop[1]));
        } catch (...) {
            return false;
        }
    }

    return !colors.empty();
}

}  // namespace linear_gradient_detail

// Host-testable parser with the same ParseFromCssLinearGradient method name as
// KRLinearGradientParser. No Harmony types; production class delegates here.
class KRLinearGradientCssParser {
 public:
    bool ParseFromCssLinearGradient(const std::string &cssGradient) {
        return linear_gradient_detail::ParseFromCssLinearGradient(cssGradient, direction, colors, locations);
    }

    const std::vector<uint32_t> &GetColors() const {
        return colors;
    }

    const std::vector<float> &GetLocations() const {
        return locations;
    }

    int GetDirection() const {
        return direction;
    }

 private:
    int direction = 0;
    std::vector<uint32_t> colors;
    std::vector<float> locations;
};

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRLINEARGRADIENTCSSPARSE_H
