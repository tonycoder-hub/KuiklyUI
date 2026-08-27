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
inline namespace model_util {

// RFC 3986 section 2.1: URI producers should use uppercase hex digits.
inline constexpr char kHexDigitsUri[] = "0123456789ABCDEF";

// leftover: KREncodeURLComponent iterated `auto &b : in` (signed char on
// OHOS aarch64/x86_64). UTF-8 high bytes (e.g. 0xE4 in "你") became
// HEX_DIGITS_URI[b / 16] with b/16 == -1 — OOB into a 16-entry table.
// Sibling KRBase64Encode already iterates unsigned char.
// Header-only so host leftover tests compile without Harmony / md5 / sha256.
inline std::string KREncodeURLComponent(const std::string &in) {
    std::string out;
    for (unsigned char b : in) {
        if (('A' <= b && b <= 'Z') || ('a' <= b && b <= 'z') || ('0' <= b && b <= '9') || b == '-' || b == '_' ||
            b == '.' || b == '!' || b == '~' || b == '*' || b == '\'' || b == '(' || b == ')') {
            out.push_back(static_cast<char>(b));
        } else {
            out.push_back('%');
            out.push_back(kHexDigitsUri[b / 16]);
            out.push_back(kHexDigitsUri[b % 16]);
        }
    }
    return out;
}

}  //  namespace util
}  //  namespace kuikly
