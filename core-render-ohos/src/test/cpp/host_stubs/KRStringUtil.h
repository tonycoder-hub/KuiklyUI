/*
 * Host-only stub of KRStringUtil.h for Color::FromString tests.
 *
 * Production KRStringUtil.h includes KRCommon.h, which pulls Harmony NDK
 * headers. Color::FromString only needs SplitString for the rgba() path.
 * This stub matches the KRStringUtil.cpp SplitString(string, string) body
 * so host g++ can include KRColor.h without a Harmony device.
 */
#ifndef CORE_RENDER_OHOS_KRSTRINGUTIL_H
#define CORE_RENDER_OHOS_KRSTRINGUTIL_H

#include <string>
#include <vector>

namespace kuikly {
namespace util {

inline std::vector<std::string> SplitString(const std::string &str, std::string separator) {
    size_t offset = 0;
    const size_t sz = str.size();
    std::vector<std::string> result;
    do {
        size_t pos = str.find(separator, offset);
        if (pos < sz) {
            result.push_back(str.substr(offset, pos - offset));
            offset = pos + separator.size();
        } else {
            break;
        }
    } while (offset < sz);
    if (offset < sz) {
        result.push_back(str.substr(offset));
    }
    return result;
}

}  // namespace util
}  // namespace kuikly

#endif  // CORE_RENDER_OHOS_KRSTRINGUTIL_H
