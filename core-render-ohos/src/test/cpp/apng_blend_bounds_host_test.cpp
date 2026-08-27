// Host unit test for leftover APNG fcTL blend / BACKGROUND dispose dstIdx OOB.
//
// leftover polarity: fcTL left/top/width/height are copied with no IHDR check.
// HandleFrameBlendOp / HandlePostFrameDisposeOp indexed
//   dstIdx = ((y + top) * canvasW + (x + left)) * 4
// without a range check. Malformed fcTL (left+width > canvas) is an OOB write.
//
// This test injects ints into the helpers (no Harmony device, no PixelMap,
// does not compile APNGStructs.cpp).
//
// Build + run (from this directory):
//   ./run_apng_blend_bounds_host_test.sh
//   ./run_apng_blend_bounds_host_test.sh asan

#include "APNGBlendBounds.h"

#include <cstdint>
#include <cstdio>
#include <limits>
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

int main() {
    const int W = 10;
    const int H = 10;
    std::vector<uint8_t> canvas(static_cast<size_t>(W) * H * 4, 0);

    // leftover: fcTL left=8 width=5 sits past canvasW=10. Old dstIdx OOB.
    {
        const int left = 8;
        const int top = 0;
        const int fw = 5;
        const int fh = 1;
        expect_true("leftover left+width past canvas is rejected",
                    !ApngFrameRectInCanvas(left, top, fw, fh, W, H));
        bool saw_oob = false;
        for (int y = 0; y < fh; ++y) {
            for (int x = 0; x < fw; ++x) {
                if (ApngLeftoverDstWouldOOB(x, y, left, top, fw, fh, W, H, canvas.size())) {
                    saw_oob = true;
                }
            }
        }
        expect_true("leftover dstIdx wrap/OOB on left=8 width=5 canvasW=10", saw_oob);
    }

    // leftover heap OOB: last scanline + left overflow runs past width*height.
    {
        const int left = 8;
        const int top = 9;
        const int fw = 5;
        const int fh = 1;
        expect_true("leftover last-row rect rejected", !ApngFrameRectInCanvas(left, top, fw, fh, W, H));
        const int64_t pix = ApngLeftoverDstIdx(4, 0, left, top, W);  // (9)*10 + 12 = 102
        expect_true("leftover last-row linear index past W*H", pix >= static_cast<int64_t>(W) * H);
        expect_true("leftover last-row dstIdx heap OOB",
                    ApngLeftoverDstWouldOOB(4, 0, left, top, fw, fh, W, H, canvas.size()));
    }

    // leftover: negative left (uint32 fcTL truncated into int).
    {
        expect_true("leftover negative left rejected", !ApngFrameRectInCanvas(-1, 0, 2, 2, W, H));
        expect_true("leftover leftover dstIdx OOB on negative left",
                    ApngLeftoverDstWouldOOB(0, 0, -1, 0, 2, 2, W, H, canvas.size()));
    }

    // leftover: top+height past canvasH.
    {
        expect_true("leftover top+height past canvas rejected", !ApngFrameRectInCanvas(0, 8, 1, 5, W, H));
    }

    // leftover: zero / negative frame size.
    {
        expect_true("leftover fw=0 rejected", !ApngFrameRectInCanvas(0, 0, 0, 1, W, H));
        expect_true("leftover fh=-1 rejected", !ApngFrameRectInCanvas(0, 0, 1, -1, W, H));
    }

    // leftover: left+fw overflows int.
    {
        const int left = std::numeric_limits<int>::max() - 2;
        expect_true("leftover left+fw int overflow rejected", !ApngFrameRectInCanvas(left, 0, 5, 1, W, H));
    }

    // in-bounds control: exact fit and interior rect.
    {
        expect_true("exact-fit rect accepted", ApngFrameRectInCanvas(5, 5, 5, 5, W, H));
        expect_true("interior rect accepted", ApngFrameRectInCanvas(0, 0, 2, 2, W, H));
        expect_true("full canvas accepted", ApngFrameRectInCanvas(0, 0, W, H, W, H));
        expect_true("in-bounds pixel not OOB",
                    !ApngLeftoverDstWouldOOB(0, 0, 0, 0, 2, 2, W, H, canvas.size()));
        expect_true("in-bounds last pixel not OOB",
                    !ApngLeftoverDstWouldOOB(1, 1, 0, 0, 2, 2, W, H, canvas.size()));
    }

    // leftover undersized canvas buffer (reserve-not-resize polarity).
    {
        std::vector<uint8_t> tiny(4, 0);
        expect_true("leftover tiny buffer rejected", !ApngCanvasBufferBigEnough(W, H, tiny.size()));
        expect_true("full canvas buffer accepted", ApngCanvasBufferBigEnough(W, H, canvas.size()));
        expect_true("zero canvas rejected", !ApngCanvasBufferBigEnough(0, H, canvas.size()));
    }

    // leftover BACKGROUND dispose uses the same dstIdx; skip when rect invalid.
    {
        expect_true("dispose BACKGROUND invalid rect skipped",
                    !ApngFrameRectInCanvas(8, 0, 5, 1, W, H));
        expect_true("dispose BACKGROUND valid rect kept", ApngFrameRectInCanvas(1, 1, 2, 2, W, H));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
