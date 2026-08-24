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

#ifndef CORE_RENDER_OHOS_APNGPARSERBUFFER_H
#define CORE_RENDER_OHOS_APNGPARSERBUFFER_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// Leftover APNG buffer helpers. subBuffer/readString indexed with no range
// check; fdAT did size_t(length - 4) (underflow when length < 4 → huge copy);
// eachChunk advanced off += 12+length without ensuring the chunk fits.
// Header-only so host g++ leftover tests compile without Harmony / ArkUI.

inline bool SubBufferInRange(size_t size, size_t start, size_t length) {
    return start <= size && length <= size - start;
}

inline std::vector<uint8_t> subBuffer(const std::vector<uint8_t> &bytes, size_t start, size_t length) {
    if (!SubBufferInRange(bytes.size(), start, length)) {
        throw std::out_of_range("subBuffer out of range");
    }
    return std::vector<uint8_t>(bytes.begin() + static_cast<std::ptrdiff_t>(start),
                                bytes.begin() + static_cast<std::ptrdiff_t>(start + length));
}

inline std::string readString(const std::vector<uint8_t> &bytes, size_t off, size_t length) {
    if (!SubBufferInRange(bytes.size(), off, length)) {
        throw std::out_of_range("readString out of range");
    }
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>(bytes[off + i]));
    }
    return result;
}

// fdAT payload is chunk data minus the 4-byte sequence number.
// length < 4 must reject: size_t(length - 4) underflows to a huge copy / crash.
inline bool tryFdATPayload(const std::vector<uint8_t> &bytes, size_t off, size_t length, std::vector<uint8_t> &out) {
    out.clear();
    if (length < 4) {
        return false;
    }
    // start = off + 8 (chunk data) + 4 (sequence number)
    if (off > bytes.size() || bytes.size() - off < 12) {
        return false;
    }
    const size_t start = off + 12;
    const size_t payload_len = length - 4;
    if (!SubBufferInRange(bytes.size(), start, payload_len)) {
        return false;
    }
    out = subBuffer(bytes, start, payload_len);
    return true;
}

inline uint32_t ReadChunkLengthBE(const std::vector<uint8_t> &bytes, size_t off) {
    return (static_cast<uint32_t>(bytes[off]) << 24) | (static_cast<uint32_t>(bytes[off + 1]) << 16) |
           (static_cast<uint32_t>(bytes[off + 2]) << 8) | static_cast<uint32_t>(bytes[off + 3]);
}

// PNG chunk: 4-byte length + 4-byte type + `length` data + 4-byte CRC.
// Stop if the 8-byte header is missing or 12+length would run past size
// (also guards size_t overflow of 12+length).
inline void eachChunk(std::vector<uint8_t> &bytes,
                      std::function<bool(const std::string &, std::vector<uint8_t> &, size_t, size_t)> callback) {
    size_t off = 8;
    while (off <= bytes.size() && bytes.size() - off >= 8) {
        const size_t length = ReadChunkLengthBE(bytes, off);
        // 12+length must not wrap size_t or run past the buffer.
        if (length > static_cast<size_t>(-1) - 12 || bytes.size() - off < 12 ||
            length > bytes.size() - off - 12) {
            break;
        }
        const std::string type = readString(bytes, off + 4, 4);
        const bool res = callback(type, bytes, off, length);
        if (!res || type == "IEND") {
            break;
        }
        off += 12 + length;
    }
}

#endif  // CORE_RENDER_OHOS_APNGPARSERBUFFER_H
