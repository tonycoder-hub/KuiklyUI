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

#ifndef CORE_RENDER_OHOS_APNG_BITMAP_BUFFER_H
#define CORE_RENDER_OHOS_APNG_BITMAP_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <vector>

// Prepare a writable leftover bitmap buffer of exactly bufferSize bytes.
//
// After clear()+reserve(), size()==0 and data() is not a writable
// bufferSize region. OH_PixelmapNative_ReadPixels writes bufferSize bytes
// into that pointer, which is leftover empty-buffer polarity (wrong result
// / heap corruption). resize() makes size()==bufferSize so data() is a
// real writable region. Host tests can call this without an OH PixelMap.
inline void PrepareBitmapBuffer(std::vector<uint8_t> &buffer, size_t bufferSize) {
    buffer.clear();
    buffer.resize(bufferSize);
}

#endif  // CORE_RENDER_OHOS_APNG_BITMAP_BUFFER_H
