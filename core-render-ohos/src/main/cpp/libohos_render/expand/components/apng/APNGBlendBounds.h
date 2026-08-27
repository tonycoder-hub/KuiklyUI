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

#ifndef CORE_RENDER_OHOS_APNG_BLEND_BOUNDS_H
#define CORE_RENDER_OHOS_APNG_BLEND_BOUNDS_H

#include <cstddef>
#include <cstdint>

// Leftover APNG fcTL blend / BACKGROUND dispose helpers.
//
// ApngParser copies fcTL width/height/left/top with no check against IHDR.
// HandleFrameBlendOp / HandlePostFrameDisposeOp then did
//   dstIdx = ((y + top) * canvasW + (x + left)) * 4
// and wrote drawingBuffer[dstIdx .. dstIdx+3] with no range check.
// A fcTL with left+width > canvasW (or negative / overflowing int) is an
// OOB write. APNG spec requires the frame rect to lie inside IHDR.
//
// Header-only so host g++ leftover tests compile without Harmony / ArkUI.

inline bool ApngCanvasBufferBigEnough(int canvasW, int canvasH, size_t bufBytes) {
    if (canvasW <= 0 || canvasH <= 0) {
        return false;
    }
    const int64_t need = static_cast<int64_t>(canvasW) * static_cast<int64_t>(canvasH) * 4;
    if (need <= 0) {
        return false;
    }
    return static_cast<uint64_t>(need) <= bufBytes;
}

// True iff the fcTL rect is entirely inside the IHDR canvas (no int overflow).
inline bool ApngFrameRectInCanvas(int left, int top, int fw, int fh, int canvasW, int canvasH) {
    if (fw <= 0 || fh <= 0 || canvasW <= 0 || canvasH <= 0) {
        return false;
    }
    if (left < 0 || top < 0) {
        return false;
    }
    const int64_t right = static_cast<int64_t>(left) + fw;
    const int64_t bottom = static_cast<int64_t>(top) + fh;
    return right <= canvasW && bottom <= canvasH;
}

// leftover dstIdx formula, exposed so host tests can show OOB polarity.
inline int64_t ApngLeftoverDstIdx(int x, int y, int left, int top, int canvasW) {
    return (static_cast<int64_t>(y) + top) * canvasW + (static_cast<int64_t>(x) + left);
}

// leftover: the old formula wraps to the next scanline when col>=canvasW,
// and is a true heap OOB when the linear index runs past width*height
// (last rows, huge left/top, or a short buffer).
inline bool ApngLeftoverDstWouldOOB(int x, int y, int left, int top, int fw, int fh, int canvasW, int canvasH,
                                    size_t bufBytes) {
    if (x < 0 || y < 0 || x >= fw || y >= fh) {
        return true;
    }
    const int64_t col = static_cast<int64_t>(x) + left;
    const int64_t row = static_cast<int64_t>(y) + top;
    if (col < 0 || row < 0 || col >= canvasW || row >= canvasH) {
        return true;
    }
    const int64_t pix = ApngLeftoverDstIdx(x, y, left, top, canvasW);
    const int64_t byte = pix * 4;
    if (byte < 0) {
        return true;
    }
    return static_cast<uint64_t>(byte) + 3 >= bufBytes;
}

#endif  // CORE_RENDER_OHOS_APNG_BLEND_BOUNDS_H
