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

#ifndef CORE_RENDER_OHOS_APNG_DISPOSE_BUFFER_H
#define CORE_RENDER_OHOS_APNG_DISPOSE_BUFFER_H

#include <cstdint>
#include <vector>

// APNG fcTL dispose_op (https://wiki.mozilla.org/APNG_Specification#fcTL:_The_Frame_Control_Chunk)
constexpr int APNG_DISPOSE_OP_NONE = 0;
constexpr int APNG_DISPOSE_OP_BACKGROUND = 1;
constexpr int APNG_DISPOSE_OP_PREVIOUS = 2;

// Save the pre-frame canvas before blending a DISPOSE_OP_PREVIOUS frame.
// Host tests inject vectors; no PixelMap / OHOS APIs.
inline void SavePreviousCanvas(std::vector<uint8_t> &previousBuffer, const std::vector<uint8_t> &curBitmapBuffer) {
    previousBuffer = curBitmapBuffer;
}

// Restore the pre-frame canvas after a DISPOSE_OP_PREVIOUS frame is displayed.
//
// leftover polarity: HandlePostFrameDisposeOp used to assign
// previousBuffer = curBitmapBuffer (overwrite the pre-blend save with the
// post-blend canvas and never read previousBuffer back). Restore is
// curBitmapBuffer = previousBuffer.
inline void RestorePreviousCanvas(std::vector<uint8_t> &curBitmapBuffer, const std::vector<uint8_t> &previousBuffer) {
    curBitmapBuffer = previousBuffer;
}

// Before blending: snapshot the canvas when this frame will revert after display.
inline void SaveCanvasIfDisposePrevious(int disposeOp, std::vector<uint8_t> &previousBuffer,
                                        const std::vector<uint8_t> &curBitmapBuffer) {
    if (disposeOp == APNG_DISPOSE_OP_PREVIOUS) {
        SavePreviousCanvas(previousBuffer, curBitmapBuffer);
    }
}

// After the frame has been displayed/committed: revert the canvas for PREVIOUS.
inline void RestoreCanvasIfDisposePrevious(int disposeOp, std::vector<uint8_t> &curBitmapBuffer,
                                           const std::vector<uint8_t> &previousBuffer) {
    if (disposeOp == APNG_DISPOSE_OP_PREVIOUS) {
        RestorePreviousCanvas(curBitmapBuffer, previousBuffer);
    }
}

#endif  // CORE_RENDER_OHOS_APNG_DISPOSE_BUFFER_H
