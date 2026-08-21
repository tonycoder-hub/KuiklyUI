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

#ifndef CORE_RENDER_OHOS_KRENTERKEYTYPEMAP_H
#define CORE_RENDER_OHOS_KRENTERKEYTYPEMAP_H

#include <string>

namespace kuikly {
namespace util {

// Integer codes match ArkUI_EnterKeyType (native_type.h / API 12+):
//   GO=2, SEARCH=3, SEND=4, NEXT=5, DONE=6, PREVIOUS=7, NEW_LINE=8
// ArkUI has no NONE. Host tests include this header without the Harmony SDK.
constexpr int kEnterKeyTypeGo = 2;
constexpr int kEnterKeyTypeSearch = 3;
constexpr int kEnterKeyTypeSend = 4;
constexpr int kEnterKeyTypeNext = 5;
constexpr int kEnterKeyTypeDone = 6;
constexpr int kEnterKeyTypePrevious = 7;
constexpr int kEnterKeyTypeNewLine = 8;

// Single-line TextInput missing-attr default. Official ArkUI enterKeyType
// default is Done. NEW_LINE was a leftover stand-in (same leftover as mapping
// "none" → NEW_LINE). Multi-line TextArea may still default to NEW_LINE.
constexpr int kEnterKeyTypeMissingAttrDefault = kEnterKeyTypeDone;

inline int MapEnterKeyTypeName(const std::string &enter_key_type) {
    if (enter_key_type == "search") {
        return kEnterKeyTypeSearch;
    }
    if (enter_key_type == "send") {
        return kEnterKeyTypeSend;
    }
    if (enter_key_type == "go") {
        return kEnterKeyTypeGo;
    }
    if (enter_key_type == "done") {
        return kEnterKeyTypeDone;
    }
    if (enter_key_type == "next") {
        return kEnterKeyTypeNext;
    }
    if (enter_key_type == "previous") {
        return kEnterKeyTypePrevious;
    }
    // Explicit newline aliases only. Do NOT map "none" → NEW_LINE.
    // Android maps "none" → IME_ACTION_NONE; ArkUI has no NONE, so fall
    // through to DONE (ArkUI default / "no special enter action").
    if (enter_key_type == "newline" || enter_key_type == "newLine") {
        return kEnterKeyTypeNewLine;
    }
    return kEnterKeyTypeDone;
}

inline const char *EnterKeyTypeCodeToName(int enter_key_type) {
    switch (enter_key_type) {
        case kEnterKeyTypeSearch:
            return "search";
        case kEnterKeyTypeSend:
            return "send";
        case kEnterKeyTypeGo:
            return "go";
        case kEnterKeyTypeDone:
            return "done";
        case kEnterKeyTypeNext:
            return "next";
        case kEnterKeyTypePrevious:
            return "previous";
        case kEnterKeyTypeNewLine:
            return "newline";
        default:
            return "";
    }
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRENTERKEYTYPEMAP_H
