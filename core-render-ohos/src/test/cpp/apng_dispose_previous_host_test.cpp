// Host unit test for leftover APNG DISPOSE_OP_PREVIOUS (disposeOp==2).
//
// leftover polarity: previousBuffer was only written. After blending,
// HandlePostFrameDisposeOp assigned previousBuffer = curBitmapBuffer, which
// overwrote the pre-blend save and never restored curBitmapBuffer.
//
// This test injects vectors into the save/restore helpers (no Harmony device,
// no OH PixelMap, does not compile APNGStructs.cpp).
//
// Build + run (from this directory):
//   ./run_apng_dispose_previous_host_test.sh
//   ./run_apng_dispose_previous_host_test.sh asan

#include "APNGDisposeBuffer.h"

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

// leftover sequence used by DidAddFrame for a non-first frame:
// save (if op==2) -> blend into cur -> restore after the frame is committed.
static void SimulateDisposeSequence(int disposeOp, std::vector<uint8_t> &cur, std::vector<uint8_t> &prev,
                                    const std::vector<uint8_t> &blended) {
    SaveCanvasIfDisposePrevious(disposeOp, prev, cur);
    cur = blended;
    RestoreCanvasIfDisposePrevious(disposeOp, cur, prev);
}

int main() {
    const std::vector<uint8_t> preFrame = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    const std::vector<uint8_t> blended = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x01, 0x02};

    // leftover op=2: restore cur to the pre-blend canvas after commit.
    {
        std::vector<uint8_t> cur = preFrame;
        std::vector<uint8_t> prev;
        SimulateDisposeSequence(APNG_DISPOSE_OP_PREVIOUS, cur, prev, blended);
        expect_true("op=2 cur restored to pre-frame", cur == preFrame);
        expect_true("op=2 previousBuffer still pre-frame", prev == preFrame);
        expect_true("op=2 previousBuffer not overwritten with blend", prev != blended);
        expect_true("op=2 cur is not left as blended", cur != blended);
    }

    // leftover overwrite polarity: the old post-op assignment would leave
    // cur==blended and previousBuffer==blended. Restore must not do that.
    {
        std::vector<uint8_t> cur = preFrame;
        std::vector<uint8_t> prev;
        SaveCanvasIfDisposePrevious(APNG_DISPOSE_OP_PREVIOUS, prev, cur);
        cur = blended;
        const std::vector<uint8_t> savedBeforeRestore = prev;
        RestoreCanvasIfDisposePrevious(APNG_DISPOSE_OP_PREVIOUS, cur, prev);
        expect_true("op=2 restore does not write previousBuffer", prev == savedBeforeRestore);
        expect_true("op=2 restore reads previousBuffer into cur", cur == savedBeforeRestore);
    }

    // leftover two-frame sequence: after op=2, the next blend starts from
    // the restored pre-frame canvas, not the committed blended frame.
    {
        std::vector<uint8_t> cur = preFrame;
        std::vector<uint8_t> prev;
        SimulateDisposeSequence(APNG_DISPOSE_OP_PREVIOUS, cur, prev, blended);
        const std::vector<uint8_t> nextBlend = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};
        SimulateDisposeSequence(APNG_DISPOSE_OP_NONE, cur, prev, nextBlend);
        expect_true("next frame after op=2 starts from restored canvas", cur == nextBlend);
        expect_true("op=0 does not clobber previousBuffer", prev == preFrame);
    }

    // leftover op=0: no save, no restore; canvas stays at the blended frame.
    {
        std::vector<uint8_t> cur = preFrame;
        std::vector<uint8_t> prev = {0xFE};
        SimulateDisposeSequence(APNG_DISPOSE_OP_NONE, cur, prev, blended);
        expect_true("op=0 cur stays blended", cur == blended);
        expect_true("op=0 previousBuffer untouched", prev.size() == 1 && prev[0] == 0xFE);
    }

    // leftover op=1: save/restore helpers are no-ops (BACKGROUND clear is
    // a separate region wipe in HandlePostFrameDisposeOp).
    {
        std::vector<uint8_t> cur = preFrame;
        std::vector<uint8_t> prev = {0xFE};
        SimulateDisposeSequence(APNG_DISPOSE_OP_BACKGROUND, cur, prev, blended);
        expect_true("op=1 helpers do not restore", cur == blended);
        expect_true("op=1 previousBuffer untouched", prev.size() == 1 && prev[0] == 0xFE);
    }

    // leftover empty pre-frame canvas: restore still copies previousBuffer.
    {
        std::vector<uint8_t> cur;
        std::vector<uint8_t> prev;
        SimulateDisposeSequence(APNG_DISPOSE_OP_PREVIOUS, cur, prev, blended);
        expect_true("empty pre-frame restore yields empty cur", cur.empty());
        expect_true("empty pre-frame previousBuffer stays empty", prev.empty());
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
