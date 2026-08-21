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

#ifndef CORE_RENDER_OHOS_KRJSONOBJECT_H
#define CORE_RENDER_OHOS_KRJSONOBJECT_H
#include <memory>
#include <string>
#include <vector>

namespace kuikly {
namespace util {

// Leftover adopt/get-string helper. cJSON_GetStringValue (or a stub with the
// same contract) returns nullptr for non-string items. Constructing
// std::string from that NULL is UB — always adopt through this helper.
inline std::string AdoptCJsonStringValue(const char *value, const std::string &default_value = "") {
    if (value == nullptr) {
        return default_value;
    }
    return std::string(value);
}

// Wrap cJSON_GetStringValue or a tiny host stub that returns nullptr for
// non-strings. item may be a real cJSON* or a stub handle.
inline std::string AdoptCJsonGetString(const char *(*get_string_value)(const void *item), const void *item,
                                       const std::string &default_value = "") {
    if (get_string_value == nullptr) {
        return default_value;
    }
    return AdoptCJsonStringValue(get_string_value(item), default_value);
}

// Array path: skip the element instead of substituting a default.
inline bool TryAdoptCJsonStringValue(const char *value, std::string *out) {
    if (value == nullptr || out == nullptr) {
        return false;
    }
    out->assign(value);
    return true;
}

class JSONObject : public std::enable_shared_from_this<JSONObject> {
 public:
    // Returns nullptr on bad JSON. Callers must check before dereference.
    static std::shared_ptr<JSONObject> Parse(const std::string &str);

    explicit JSONObject(void *cjson, std::shared_ptr<JSONObject> owner = nullptr);
    ~JSONObject();

    std::string GetString(const std::string &key, const std::string &default_value = "");
    std::vector<std::string> GetStringArray(const std::string &key);

    double GetNumber(const std::string &key, const double default_value = 0);
    std::vector<double> GetNumberArray(const std::string &key);

    std::shared_ptr<JSONObject> GetArrayItem(int index);
    int GetArraySize();

    std::shared_ptr<JSONObject> GetObjectItem(const std::string &key);

 private:
    void *cjson_ = nullptr;
    std::shared_ptr<JSONObject> owner_ = nullptr;
};

}  // end namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRJSONOBJECT_H
