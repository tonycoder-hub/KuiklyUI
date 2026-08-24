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

#ifndef CORE_RENDER_OHOS_APNG_CANVAS_INDEX_H
#define CORE_RENDER_OHOS_APNG_CANVAS_INDEX_H

#include <cstddef>
#include <cstdint>
#include <vector>

// Header-only leftover helpers for APNG blend / dispose-clear dstIdx.
//
// Parser fcTL left/top/width/height are not clamped to the canvas. The
// leftover compositor used:
//   dstIdx = ((y + frame->top) * width + (x + frame->left)) * 4
// then wrote drawingBuffer[dstIdx..+3]. A frame with left=8, width=5 on
// canvas W=10 walks past the RGBA buffer. Same math in dispose-clear.
//
// Host tests can include this file without Harmony / OH PixelMap.

// True iff the fcTL rectangle is entirely inside the canvas:
// left>=0 && top>=0 && left+frameWidth<=canvasWidth &&
// top+frameHeight<=canvasHeight (and dimensions are non-negative).
inline bool IsFrameRectInCanvas(int left, int top, int frameWidth, int frameHeight, int canvasWidth,
                                int canvasHeight) {
    if (left < 0 || top < 0 || frameWidth < 0 || frameHeight < 0) {
        return false;
    }
    if (canvasWidth < 0 || canvasHeight < 0) {
        return false;
    }
    if (static_cast<int64_t>(left) + static_cast<int64_t>(frameWidth) > canvasWidth) {
        return false;
    }
    if (static_cast<int64_t>(top) + static_cast<int64_t>(frameHeight) > canvasHeight) {
        return false;
    }
    return true;
}

// RGBA8888 dest index. Caller must have accepted the frame via
// IsFrameRectInCanvas and must pass x in [0, frameWidth), y in [0, frameHeight).
inline size_t CanvasDstIndex(int x, int y, int left, int top, int canvasWidth) {
    const size_t px = static_cast<size_t>(x) + static_cast<size_t>(left);
    const size_t py = static_cast<size_t>(y) + static_cast<size_t>(top);
    return (py * static_cast<size_t>(canvasWidth) + px) * 4u;
}

// Per-pixel leftover clamp: false if (x+left, y+top) is outside the canvas
// or dstIdx..+3 would exceed canvasBytes.
inline bool TryCanvasDstIndex(int x, int y, int left, int top, int canvasWidth, int canvasHeight, size_t canvasBytes,
                              size_t *outIdx) {
    if (outIdx == nullptr || x < 0 || y < 0) {
        return false;
    }
    const int64_t px = static_cast<int64_t>(x) + static_cast<int64_t>(left);
    const int64_t py = static_cast<int64_t>(y) + static_cast<int64_t>(top);
    if (px < 0 || py < 0 || px >= canvasWidth || py >= canvasHeight) {
        return false;
    }
    const size_t idx =
        (static_cast<size_t>(py) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(px)) * 4u;
    if (idx + 4u > canvasBytes) {
        return false;
    }
    *outIdx = idx;
    return true;
}

// SOURCE copy (blendOp==0) of a frame buffer onto a host vector canvas.
// Returns false and leaves canvas unchanged if the frame rect is OOB.
inline bool BlendSourceFrameOntoCanvas(std::vector<uint8_t> &canvas, int canvasWidth, int canvasHeight,
                                       const std::vector<uint8_t> &frameBuffer, int left, int top, int frameWidth,
                                       int frameHeight) {
    if (!IsFrameRectInCanvas(left, top, frameWidth, frameHeight, canvasWidth, canvasHeight)) {
        return false;
    }
    const size_t needSrc = static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * 4u;
    const size_t needDst = static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight) * 4u;
    if (frameBuffer.size() < needSrc || canvas.size() < needDst) {
        return false;
    }
    for (int y = 0; y < frameHeight; ++y) {
        for (int x = 0; x < frameWidth; ++x) {
            const size_t srcIdx = (static_cast<size_t>(y) * static_cast<size_t>(frameWidth) + static_cast<size_t>(x)) * 4u;
            const size_t dstIdx = CanvasDstIndex(x, y, left, top, canvasWidth);
            canvas[dstIdx + 0] = frameBuffer[srcIdx + 0];
            canvas[dstIdx + 1] = frameBuffer[srcIdx + 1];
            canvas[dstIdx + 2] = frameBuffer[srcIdx + 2];
            canvas[dstIdx + 3] = frameBuffer[srcIdx + 3];
        }
    }
    return true;
}

// disposeOp BACKGROUND (1): clear the frame rectangle. Returns false and
// leaves canvas unchanged if the frame rect is OOB.
inline bool DisposeClearFrameOnCanvas(std::vector<uint8_t> &canvas, int canvasWidth, int canvasHeight, int left,
                                      int top, int frameWidth, int frameHeight) {
    if (!IsFrameRectInCanvas(left, top, frameWidth, frameHeight, canvasWidth, canvasHeight)) {
        return false;
    }
    const size_t needDst = static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight) * 4u;
    if (canvas.size() < needDst) {
        return false;
    }
    for (int y = 0; y < frameHeight; ++y) {
        for (int x = 0; x < frameWidth; ++x) {
            const size_t dstIdx = CanvasDstIndex(x, y, left, top, canvasWidth);
            canvas[dstIdx + 0] = 0;
            canvas[dstIdx + 1] = 0;
            canvas[dstIdx + 2] = 0;
            canvas[dstIdx + 3] = 0;
        }
    }
    return true;
}

#endif  // CORE_RENDER_OHOS_APNG_CANVAS_INDEX_H
