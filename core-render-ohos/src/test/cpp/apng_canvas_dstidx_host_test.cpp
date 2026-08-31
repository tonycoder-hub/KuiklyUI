// Host unit test for leftover APNG blend/dispose canvas dstIdx OOB.
//
// leftover polarity: fcTL left/top/width/height are unclamped. The compositor
// computed dstIdx = ((y + top) * width + (x + left)) * 4 and wrote
// drawingBuffer[dstIdx..+3]. left=8, width=5 on canvas W=10 walks past the
// RGBA buffer. Same math in dispose-clear.
//
// This test drives the header-only index/clamp helpers over a std::vector
// canvas (no Harmony device, no OH PixelMap, does not compile APNGStructs.cpp).
//
// Build + run (from this directory):
//   ./run_apng_canvas_dstidx_host_test.sh
//   ./run_apng_canvas_dstidx_host_test.sh asan

#include "APNGCanvasIndex.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static int g_failed = 0;

static void expect_true(const char *name, bool cond) {
    if (!cond) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq_size(const char *name, size_t got, size_t want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %zu want %zu\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// leftover dstIdx formula from HandleFrameBlendOp / dispose-clear.
static int LeftoverDstIdx(int x, int y, int left, int top, int canvasWidth) {
    return ((y + top) * canvasWidth + (x + left)) * 4;
}

static bool LeftoverWriteWalksPast(int left, int top, int frameWidth, int frameHeight, int canvasWidth,
                                   int canvasHeight) {
    const int canvasBytes = canvasWidth * canvasHeight * 4;
    for (int y = 0; y < frameHeight; ++y) {
        for (int x = 0; x < frameWidth; ++x) {
            const int dstIdx = LeftoverDstIdx(x, y, left, top, canvasWidth);
            if (dstIdx < 0 || dstIdx + 3 >= canvasBytes) {
                return true;
            }
        }
    }
    return false;
}

int main() {
    // leftover example: left=8, width=5 on W=10 walks past the buffer.
    // On a 1-row canvas the leftover dstIdx for x=4 is 12*4=48, past 40 bytes.
    // On a taller canvas the same x wraps into the next row (wrong pixels)
    // and only the last row walks off the end.
    {
        const int canvasW = 10;
        const int canvasH = 1;
        const int left = 8;
        const int top = 0;
        const int fw = 5;
        const int fh = 1;
        expect_true("leftover left=8 width=5 on W=10 walks past",
                    LeftoverWriteWalksPast(left, top, fw, fh, canvasW, canvasH));
        expect_true("OOB left=8 width=5 detected", !IsFrameRectInCanvas(left, top, fw, fh, canvasW, canvasH));

        std::vector<uint8_t> canvas(static_cast<size_t>(canvasW * canvasH * 4), 0xCC);
        const std::vector<uint8_t> before = canvas;
        std::vector<uint8_t> frame(static_cast<size_t>(fw * fh * 4), 0x11);
        expect_true("OOB SOURCE blend rejected",
                    !BlendSourceFrameOntoCanvas(canvas, canvasW, canvasH, frame, left, top, fw, fh));
        expect_true("OOB SOURCE blend leaves canvas unchanged", canvas == before);
        expect_true("OOB dispose-clear rejected",
                    !DisposeClearFrameOnCanvas(canvas, canvasW, canvasH, left, top, fw, fh));
        expect_true("OOB dispose-clear leaves canvas unchanged", canvas == before);
    }

    // leftover vertical OOB: top=8, height=5 on H=10.
    {
        expect_true("OOB top=8 height=5 on H=10 detected", !IsFrameRectInCanvas(0, 8, 1, 5, 10, 10));
        expect_true("leftover top=8 height=5 walks past", LeftoverWriteWalksPast(0, 8, 1, 5, 10, 10));
    }

    // leftover last-row wrap: left=8,width=5 on W=10,H=4 writes past the last row.
    {
        expect_true("leftover last-row left=8 width=5 walks past", LeftoverWriteWalksPast(8, 3, 5, 1, 10, 4));
        expect_true("OOB last-row left=8 width=5 detected", !IsFrameRectInCanvas(8, 3, 5, 1, 10, 4));
    }

    // leftover negative origin (uint32 overflow into signed int).
    {
        expect_true("negative left rejected", !IsFrameRectInCanvas(-1, 0, 2, 2, 10, 10));
        expect_true("negative top rejected", !IsFrameRectInCanvas(0, -1, 2, 2, 10, 10));
        expect_true("negative frame width rejected", !IsFrameRectInCanvas(0, 0, -1, 2, 10, 10));
    }

    // leftover exact-fit and in-bounds frames are accepted.
    {
        expect_true("full-canvas frame in bounds", IsFrameRectInCanvas(0, 0, 10, 10, 10, 10));
        expect_true("left+width == canvasW accepted", IsFrameRectInCanvas(5, 0, 5, 1, 10, 4));
        expect_true("left+width > canvasW rejected", !IsFrameRectInCanvas(6, 0, 5, 1, 10, 4));
        expect_true("zero-size at origin accepted", IsFrameRectInCanvas(0, 0, 0, 0, 10, 10));
    }

    // leftover in-bounds SOURCE write: left=2, width=3 on W=10.
    {
        const int canvasW = 10;
        const int canvasH = 2;
        const int left = 2;
        const int top = 0;
        const int fw = 3;
        const int fh = 1;
        expect_true("in-bounds left=2 width=3 accepted", IsFrameRectInCanvas(left, top, fw, fh, canvasW, canvasH));
        expect_true("in-bounds leftover formula stays inside",
                    !LeftoverWriteWalksPast(left, top, fw, fh, canvasW, canvasH));

        std::vector<uint8_t> canvas(static_cast<size_t>(canvasW * canvasH * 4), 0xCC);
        std::vector<uint8_t> frame(static_cast<size_t>(fw * fh * 4), 0);
        // RGB pixels: (R,G,B,A) = (1,2,3,4), (5,6,7,8), (9,10,11,12)
        for (int i = 0; i < fw; ++i) {
            frame[static_cast<size_t>(i) * 4u + 0] = static_cast<uint8_t>(1 + i * 4);
            frame[static_cast<size_t>(i) * 4u + 1] = static_cast<uint8_t>(2 + i * 4);
            frame[static_cast<size_t>(i) * 4u + 2] = static_cast<uint8_t>(3 + i * 4);
            frame[static_cast<size_t>(i) * 4u + 3] = static_cast<uint8_t>(4 + i * 4);
        }
        expect_true("in-bounds SOURCE blend writes",
                    BlendSourceFrameOntoCanvas(canvas, canvasW, canvasH, frame, left, top, fw, fh));

        for (int i = 0; i < fw; ++i) {
            const size_t dstIdx = CanvasDstIndex(i, 0, left, top, canvasW);
            expect_eq_size("in-bounds dstIdx matches leftover formula", dstIdx,
                           static_cast<size_t>(LeftoverDstIdx(i, 0, left, top, canvasW)));
            expect_true("in-bounds pixel written", canvas[dstIdx + 0] == static_cast<uint8_t>(1 + i * 4) &&
                                                       canvas[dstIdx + 1] == static_cast<uint8_t>(2 + i * 4) &&
                                                       canvas[dstIdx + 2] == static_cast<uint8_t>(3 + i * 4) &&
                                                       canvas[dstIdx + 3] == static_cast<uint8_t>(4 + i * 4));
        }
        // Pixel immediately left of the frame stays 0xCC.
        expect_true("pixel left of frame untouched", canvas[CanvasDstIndex(0, 0, 1, 0, canvasW)] == 0xCC);
        // Pixel immediately right of the frame stays 0xCC (x=5).
        expect_true("pixel right of frame untouched", canvas[CanvasDstIndex(0, 0, 5, 0, canvasW)] == 0xCC);
        // Second row untouched.
        expect_true("second row untouched", canvas[CanvasDstIndex(0, 0, left, 1, canvasW)] == 0xCC);
    }

    // leftover in-bounds dispose-clear zeros only the frame rect.
    {
        const int canvasW = 6;
        const int canvasH = 3;
        const int left = 1;
        const int top = 1;
        const int fw = 2;
        const int fh = 1;
        std::vector<uint8_t> canvas(static_cast<size_t>(canvasW * canvasH * 4), 0xAB);
        expect_true("in-bounds dispose-clear accepted",
                    DisposeClearFrameOnCanvas(canvas, canvasW, canvasH, left, top, fw, fh));
        for (int x = 0; x < fw; ++x) {
            const size_t dstIdx = CanvasDstIndex(x, 0, left, top, canvasW);
            expect_true("dispose-clear zeroed frame pixel", canvas[dstIdx + 0] == 0 && canvas[dstIdx + 1] == 0 &&
                                                               canvas[dstIdx + 2] == 0 && canvas[dstIdx + 3] == 0);
        }
        expect_true("dispose-clear left neighbor untouched", canvas[CanvasDstIndex(0, 0, 0, 1, canvasW)] == 0xAB);
        expect_true("dispose-clear row 0 untouched", canvas[0] == 0xAB);
    }

    // leftover TryCanvasDstIndex clips per-pixel (alternative to reject).
    {
        size_t idx = 999;
        const size_t canvasBytes = static_cast<size_t>(10 * 2 * 4);
        expect_true("TryCanvasDstIndex in-bounds", TryCanvasDstIndex(0, 0, 2, 0, 10, 2, canvasBytes, &idx));
        expect_eq_size("TryCanvasDstIndex idx", idx, 2u * 4u);
        expect_true("TryCanvasDstIndex OOB left=8 width walk",
                    !TryCanvasDstIndex(4, 0, 8, 0, 10, 2, canvasBytes, &idx));
        expect_true("TryCanvasDstIndex null out rejected",
                    !TryCanvasDstIndex(0, 0, 0, 0, 10, 2, canvasBytes, nullptr));
    }

    // leftover int overflow: huge left+width must not wrap and accept.
    {
        expect_true("INT_MAX left + width rejected", !IsFrameRectInCanvas(2147483647, 0, 2, 1, 10, 10));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
