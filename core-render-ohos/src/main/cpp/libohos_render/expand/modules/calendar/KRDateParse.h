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
#pragma once

#include <string>

namespace kuikly {
namespace util {

// Leftover Date::Parse helpers. Bare formatStr.find + quoteCount subtract +
// std::stoi(dateStr.substr(pos, N)) has no pos/width check: a short date
// ("1" vs "yyyy-MM-dd") throws std::out_of_range, and a non-numeric token
// ("xxxx" vs "yyyy") throws std::invalid_argument. Header-only so host g++
// leftover tests compile without Harmony / ArkUI.

// Count "'" / "''" in the prefix before a format token (Android date-pattern
// convention). Used to map format position onto the quote-stripped date string.
inline std::string::size_type DateParseQuoteCount(const std::string &subString) {
    std::string::size_type count = 0;
    auto length = subString.length();
    for (std::string::size_type i = 0; i < length; i++) {
        if (subString.at(i) == '\'') {
            if ((i + 1) < length && subString.at(i + 1) == '\'') {
                count++;  //  双引号计数。'' is treated as a single quote regardless of being in a quoted section.
                i++;
                continue;
            }
            count++;  //  单引号计数
        }
    }
    return count;
}

// Parse one format token (yyyy/YYYY/MM/dd/HH/mm/ss/SSS). Returns true and
// writes `out` only when the token is present, the adjusted pos is in range,
// and the slice is numeric. On any failure returns false without throwing
// (caller leaves existing Date members).
inline bool TryParseDateField(const std::string &dateStr, const std::string &formatStr, const char *token,
                              std::string::size_type width, int &out) {
    auto pos = formatStr.find(token);
    if (pos == std::string::npos) {
        return false;
    }
    const auto quotes = DateParseQuoteCount(formatStr.substr(0, pos));
    // size_type is unsigned: quoteCount > pos would wrap to a huge index.
    if (quotes > pos) {
        return false;
    }
    pos -= quotes;
    if (pos >= dateStr.size() || pos + width > dateStr.size()) {
        return false;
    }
    try {
        out = std::stoi(dateStr.substr(pos, width));
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace util
}  // namespace kuikly
