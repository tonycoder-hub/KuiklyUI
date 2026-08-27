// Host unit test for leftover KRSnapshotManager dataUri pixelMap / malloc guards.
//
// Leftover TakeSnapshot dataUri path:
//   drawableDescriptor was null-checked; pixelMap was not.
//   ProcessSnapshotResultWithDataType then InitNativePixelMap / GetImageInfo /
//   PackToData on a null/undefined pixelMap.
//
// Leftover ProcessSnapshotResultWithDataType:
//   size = width * height * 4; malloc(size);
//   no overflow / zero / malloc-null check.
//
// Production cpp cannot host-compile without Harmony. Header-only
// KRSnapshotDataUriGuards.h extracts the size / null guards.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_snapshot_datauri_pixelmap_test.sh
//   ./run_kr_snapshot_datauri_pixelmap_test.sh asan

#include "KRSnapshotDataUriGuards.h"

#include <cstdio>
#include <cstdint>
#include <limits>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_eq(const char *name, int got, int want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    // leftover: width * height overflow must reject (no wrap into malloc).
    {
        size_t size = 0xdeadbeef;
        bool ok = KRSnapshotDataUriComputePackedSize(std::numeric_limits<uint32_t>::max(),
                                                     std::numeric_limits<uint32_t>::max(), &size);
        expect_true("width*height overflow reject", !ok);
        expect_true("overflow leaves out_size untouched", size == 0xdeadbeef);
    }
    {
        size_t size = 0xdeadbeef;
        // 2^31 * 2^31 = 2^62; 2^62 * 4 overflows size_t on 64-bit.
        bool ok = KRSnapshotDataUriComputePackedSize(1u << 31, 1u << 31, &size);
        expect_true("2^31 * 2^31 * 4 overflow reject", !ok);
        expect_true("2^31 overflow leaves out_size untouched", size == 0xdeadbeef);
    }

    // leftover: size == 0 (zero width or height) must reject.
    {
        size_t size = 0xdeadbeef;
        expect_true("width==0 reject", !KRSnapshotDataUriComputePackedSize(0, 10, &size));
        expect_true("height==0 reject", !KRSnapshotDataUriComputePackedSize(10, 0, &size));
        expect_true("0x0 reject", !KRSnapshotDataUriComputePackedSize(0, 0, &size));
        expect_true("zero size leaves out_size untouched", size == 0xdeadbeef);
    }

    // leftover: null / undefined pixelMap → code = -1, never ProcessSnapshotResultWithDataType.
    {
        expect_true("null pixelMap rejected", KRSnapshotDataUriPixelMapRejected(true, false));
        expect_true("undefined pixelMap rejected", KRSnapshotDataUriPixelMapRejected(false, true));
        expect_true("null+undefined pixelMap rejected", KRSnapshotDataUriPixelMapRejected(true, true));
        expect_true("present pixelMap not rejected", !KRSnapshotDataUriPixelMapRejected(false, false));

        KRSnapshotDataUriResult null_result = KRSnapshotDataUriRejectNullPixelMap();
        expect_eq("null-pixelMap code=-1", null_result.code, -1);
        expect_true("null-pixelMap message set", !null_result.message.empty());
    }

    // leftover: valid size still computes width * height * 4 (success path unchanged).
    {
        size_t size = 0;
        expect_true("1x1 packed size ok", KRSnapshotDataUriComputePackedSize(1, 1, &size));
        expect_true("1x1 packed size == 4", size == 4);
        expect_true("10x20 packed size ok", KRSnapshotDataUriComputePackedSize(10, 20, &size));
        expect_true("10x20 packed size == 800", size == 800);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover snapshot dataUri pixelMap/malloc tests passed\n");
    return 0;
}
