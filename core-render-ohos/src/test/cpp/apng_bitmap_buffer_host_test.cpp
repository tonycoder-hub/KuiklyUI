// Host unit test for leftover APNG PixelmapToBitmapBuffer empty-buffer polarity.
//
// After clear()+reserve(), size()==0 and data() is not a writable bufferSize
// region. PrepareBitmapBuffer must resize so size()==bufferSize and
// capacity()>=bufferSize. This test does not call OH_PixelmapNative_ReadPixels;
// it only exercises the leftover prep helper (no Harmony device / OH PixelMap).
//
// Build + run (from this directory):
//   ./run_apng_bitmap_buffer_host_test.sh
//   ./run_apng_bitmap_buffer_host_test.sh asan

#include "APNGBitmapBuffer.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

// Simulate leftover ReadPixels writing bufferSize bytes into data().
// ASan/UBSan would catch leftover reserve polarity (write past size()==0).
static void SimulateReadPixels(std::vector<uint8_t> &buffer, size_t bufferSize) {
    if (bufferSize == 0) {
        return;
    }
    std::memset(buffer.data(), 0xAB, bufferSize);
}

static void AssertPrepared(const char *label, const std::vector<uint8_t> &buffer, size_t bufferSize) {
    char sizeName[128];
    char capName[128];
    std::snprintf(sizeName, sizeof(sizeName), "%s size==bufferSize", label);
    std::snprintf(capName, sizeof(capName), "%s capacity>=bufferSize", label);
    expect_eq_size(sizeName, buffer.size(), bufferSize);
    expect_true(capName, buffer.capacity() >= bufferSize);
}

int main() {
    // leftover empty-buffer: fresh vector, typical 2x2 RGBA frame.
    {
        const size_t bufferSize = 2 * 2 * 4;
        std::vector<uint8_t> buffer;
        PrepareBitmapBuffer(buffer, bufferSize);
        AssertPrepared("fresh 16", buffer, bufferSize);
        SimulateReadPixels(buffer, bufferSize);
        expect_eq_size("fresh 16 after simulated ReadPixels size", buffer.size(), bufferSize);
        expect_true("fresh 16 data[0] written", buffer[0] == 0xAB);
        expect_true("fresh 16 data[last] written", buffer[bufferSize - 1] == 0xAB);
    }

    // leftover reuse of a larger buffer must shrink size to bufferSize.
    {
        const size_t bufferSize = 8;
        std::vector<uint8_t> buffer(64, 0xFF);
        PrepareBitmapBuffer(buffer, bufferSize);
        AssertPrepared("reuse-shrink 8", buffer, bufferSize);
        SimulateReadPixels(buffer, bufferSize);
        expect_true("reuse-shrink cleared leftover prefix", buffer[0] == 0xAB);
    }

    // leftover reuse of a smaller buffer must grow size to bufferSize.
    {
        const size_t bufferSize = 32;
        std::vector<uint8_t> buffer(4, 0xFF);
        PrepareBitmapBuffer(buffer, bufferSize);
        AssertPrepared("reuse-grow 32", buffer, bufferSize);
        SimulateReadPixels(buffer, bufferSize);
        expect_true("reuse-grow last byte writable", buffer[bufferSize - 1] == 0xAB);
    }

    // leftover zero-size prep: size==0 is correct; data() is not written.
    {
        std::vector<uint8_t> buffer(12, 0xFF);
        PrepareBitmapBuffer(buffer, 0);
        AssertPrepared("zero", buffer, 0);
        SimulateReadPixels(buffer, 0);
        expect_eq_size("zero size stays 0", buffer.size(), 0);
    }

    // leftover 1x1 RGBA (4 bytes) — smallest real PixelmapToBitmapBuffer size.
    {
        const size_t bufferSize = 1 * 1 * 4;
        std::vector<uint8_t> buffer;
        PrepareBitmapBuffer(buffer, bufferSize);
        AssertPrepared("1x1 rgba", buffer, bufferSize);
        SimulateReadPixels(buffer, bufferSize);
        expect_eq_size("1x1 rgba after write", buffer.size(), bufferSize);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
