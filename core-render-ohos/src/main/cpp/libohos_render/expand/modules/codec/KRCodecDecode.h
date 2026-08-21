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

#include <cctype>
#include <string>

namespace kuikly {
inline namespace model_util {

// Decode one leftover hex nibble. C isxdigit/isdigit/toupper require
// unsigned char or EOF; leftover KRDecodeURLComponent passed raw char
// (signed on OHOS aarch64/x86_64), so high bytes like 0x80 were UB.
// Returns 0..15, or -1 when c is not a hex digit.
inline int KRDecodeHexNibble(unsigned char c) {
    if (!std::isxdigit(c)) {
        return -1;
    }
    if (std::isdigit(c)) {
        return static_cast<int>(c - '0');
    }
    return static_cast<int>(std::toupper(c) - 'A' + 10);
}

// Percent-decode. Invalid %xx (including leftover high bytes) is kept as
// a literal '%'; the following two bytes are then scanned as usual.
inline std::string KRDecodeURLComponent(const std::string &in) {
    std::string res;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            unsigned char d1 = static_cast<unsigned char>(in[i + 1]);
            unsigned char d2 = static_cast<unsigned char>(in[i + 2]);
            int n1 = KRDecodeHexNibble(d1);
            int n2 = KRDecodeHexNibble(d2);
            if (n1 >= 0 && n2 >= 0) {
                res.push_back(static_cast<char>((n1 << 4) | n2));
                i += 2;
            } else {
                res.push_back(in[i]);
            }
        } else {
            res.push_back(in[i]);
        }
    }
    return res;
}

}  //  namespace util
}  //  namespace kuikly
