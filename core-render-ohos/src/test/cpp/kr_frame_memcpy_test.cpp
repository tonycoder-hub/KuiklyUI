// Host unit test for leftover KRBasePropsHandler kFrame memcpy length.
//
// The kFrame branch used to:
//   memcpy(&frame, s.data(), s.size());
// into a stack KRRect (4 floats + trailing isDefault_). Caller-controlled
// s.size() > sizeof(frame) is a stack smash; a text frame string writes
// garbage. Helper accepts only exactly 4 floats and copies into x/y/width/
// height so isDefault_ is not clobbered. Header-only KRFrameMemcpy.h so this
// compiles without ArkUI / Harmony.
//
// Build + run (from this directory):
//   ./run_kr_frame_memcpy_test.sh
//   ./run_kr_frame_memcpy_test.sh asan

#include "libohos_render/utils/KRFrameMemcpy.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

using kuikly::util::kKRFrameFloatBytes;
using kuikly::util::TryCopyFrameFloats;

static int g_failed = 0;

// KRRect-like: 4 floats plus the trailing flag the leftover memcpy used to
// overwrite when s.size() >= sizeof(KRRect).
struct FrameLike {
    float x = 9.0f;
    float y = 9.0f;
    float width = 9.0f;
    float height = 9.0f;
    bool isDefault_ = true;
    unsigned char canary[8] = {'C', 'A', 'N', 'A', 'R', 'Y', '!', 0};
};

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static bool floats_eq(const FrameLike &got, float x, float y, float w, float h) {
    return got.x == x && got.y == y && got.width == w && got.height == h;
}

static bool canary_intact(const FrameLike &got) {
    return std::memcmp(got.canary, "CANARY!", 8) == 0;
}

static std::string pack4(float x, float y, float w, float h) {
    float xywh[4] = {x, y, w, h};
    return std::string(reinterpret_cast<const char *>(xywh), sizeof(xywh));
}

int main() {
    expect_true("kKRFrameFloatBytes == 16", kKRFrameFloatBytes == 16);
    expect_true("FrameLike larger than 4 floats", sizeof(FrameLike) > kKRFrameFloatBytes);

    // leftover: size 0 must reject and not write the dest.
    {
        FrameLike frame;
        expect_true("size 0 reject", !TryCopyFrameFloats(std::string_view(), frame));
        expect_true("size 0 dest unchanged", floats_eq(frame, 9, 9, 9, 9));
        expect_true("size 0 isDefault_ intact", frame.isDefault_);
        expect_true("size 0 canary intact", canary_intact(frame));
    }

    // leftover: size 15 (one byte short) must reject / no overflow.
    {
        FrameLike frame;
        std::string buf(15, '\x7f');
        expect_true("size 15 reject", !TryCopyFrameFloats(buf, frame));
        expect_true("size 15 dest unchanged", floats_eq(frame, 9, 9, 9, 9));
        expect_true("size 15 isDefault_ intact", frame.isDefault_);
        expect_true("size 15 canary intact", canary_intact(frame));
    }

    // leftover: size 64 used to smash past the stack KRRect. Reject, no overflow.
    {
        FrameLike frame;
        std::string buf(64, '\xff');
        expect_true("size 64 reject", !TryCopyFrameFloats(buf, frame));
        expect_true("size 64 dest unchanged", floats_eq(frame, 9, 9, 9, 9));
        expect_true("size 64 isDefault_ intact", frame.isDefault_);
        expect_true("size 64 canary intact", canary_intact(frame));
    }

    // leftover: text frame string is not a 16-byte float payload.
    {
        FrameLike frame;
        expect_true("text frame reject", !TryCopyFrameFloats(std::string_view("10 20 30 40"), frame));
        expect_true("text frame dest unchanged", floats_eq(frame, 9, 9, 9, 9));
    }

    // size == 16 copies the 4 floats and must not clobber isDefault_ / canary.
    {
        FrameLike frame;
        const std::string payload = pack4(1.5f, 2.5f, 3.5f, 4.5f);
        expect_true("size 16 payload", payload.size() == 16);
        expect_true("size 16 accept", TryCopyFrameFloats(payload, frame));
        expect_true("size 16 floats", floats_eq(frame, 1.5f, 2.5f, 3.5f, 4.5f));
        expect_true("size 16 isDefault_ intact", frame.isDefault_);
        expect_true("size 16 canary intact", canary_intact(frame));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
