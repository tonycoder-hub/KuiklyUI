// Host unit test for leftover custom-font OpenRawFile null / len<=0.
//
// Leftover (KRParagraph LoadCustomFont + KRRichTextShadow RegisterCustomFont):
//   RawFile *rawFile = OH_ResourceManager_OpenRawFile(resMgr, path);
//   long len = OH_ResourceManager_GetRawFileSize(rawFile);
//   std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(len);
//
// OpenRawFile returns NULL on missing rawfile / null resMgr. GetRawFileSize
// on that pointer is -1. make_unique<uint8_t[]>(-1) is size_t max → bad_alloc
// (or asan abort). First Text/RichText layout with fontFamily adapter
// returning rawfile:fonts/Missing.ttf kills the process.
//
// Fix:
//   if (!rawFile) skip (no size/read/close, no alloc)
//   if (len <= 0) skip (CloseRawFile if opened; do not allocate)
//   RegisterCustomFont: reject null resMgr at start (like KRParagraph L77)
//   caller: skip RegisterCustomFont if !rootViewLock; still set font families
//
// KRParagraph.cpp / KRRichTextShadow.cpp cannot host-compile without NDK.
// Helper kr_custom_font_rawfile_guard.h mirrors the FIXED decision.
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_custom_font_rawfile_null_test.sh
//   ./run_kr_custom_font_rawfile_null_test.sh asan

#include "kr_custom_font_rawfile_guard.h"

#include <cstdio>
#include <limits>

using kuikly::leftover_custom_font::leftoverLenConvertsToSizeTMax;
using kuikly::leftover_custom_font::PlanRawFileLoad;
using kuikly::leftover_custom_font::RawFileAction;
using kuikly::leftover_custom_font::ShouldCallRegisterCustomFont;
using kuikly::leftover_custom_font::ShouldRegisterCustomFont;
using kuikly::leftover_custom_font::ShouldSetFontFamilies;

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// leftover polarity: GetRawFileSize -1 → unique_ptr<uint8_t[]>(size_t max).
static void leftover_len_minus_one_is_size_t_max() {
    expect_true("leftover len==-1 converts to size_t max", leftoverLenConvertsToSizeTMax(-1));
    expect_true("leftover len==0 is not size_t max", !leftoverLenConvertsToSizeTMax(0));
    expect_true("leftover len==16 is not size_t max", !leftoverLenConvertsToSizeTMax(16));
}

int main() {
    leftover_len_minus_one_is_size_t_max();

    // leftover: null rawFile → skip (no size/read/close, no alloc).
    {
        auto p = PlanRawFileLoad(nullptr, -1);
        expect_true("null rawFile action SkipNullRawFile", p.action == RawFileAction::SkipNullRawFile);
        expect_true("null rawFile no get size", !p.would_get_size);
        expect_true("null rawFile no read", !p.would_read);
        expect_true("null rawFile no close", !p.would_close);
        expect_true("null rawFile no alloc", !p.would_allocate);
        expect_true("null rawFile planned_bytes 0", p.planned_bytes == 0);
    }

    // leftover: len<=0 → skip, CloseRawFile if opened, no alloc.
    {
        const char opened = 1;
        auto p0 = PlanRawFileLoad(&opened, 0);
        expect_true("len==0 action SkipNonPositive", p0.action == RawFileAction::SkipNonPositive);
        expect_true("len==0 get size", p0.would_get_size);
        expect_true("len==0 no read", !p0.would_read);
        expect_true("len==0 close", p0.would_close);
        expect_true("len==0 no alloc", !p0.would_allocate);
        expect_true("len==0 planned_bytes 0", p0.planned_bytes == 0);

        auto pneg = PlanRawFileLoad(&opened, -1);
        expect_true("len==-1 action SkipNonPositive", pneg.action == RawFileAction::SkipNonPositive);
        expect_true("len==-1 no alloc (not size_t max)", !pneg.would_allocate);
        expect_true("len==-1 planned_bytes 0 (not size_t max)", pneg.planned_bytes == 0);
        expect_true("len==-1 planned_bytes != size_t max",
                    pneg.planned_bytes != std::numeric_limits<std::size_t>::max());
        expect_true("len==-1 close", pneg.would_close);
        expect_true("len==-1 no read", !pneg.would_read);
    }

    // leftover: len>0 → would allocate that many bytes (no size_t max).
    {
        const char opened = 1;
        auto p = PlanRawFileLoad(&opened, 16);
        expect_true("len>0 action Load", p.action == RawFileAction::Load);
        expect_true("len>0 get size", p.would_get_size);
        expect_true("len>0 read", p.would_read);
        expect_true("len>0 close", p.would_close);
        expect_true("len>0 allocate", p.would_allocate);
        expect_true("len>0 planned_bytes 16", p.planned_bytes == 16);
        expect_true("len>0 planned_bytes != size_t max",
                    p.planned_bytes != std::numeric_limits<std::size_t>::max());
    }

    // leftover: RegisterCustomFont rejects null resMgr / empty family.
    expect_true("null resMgr skip register", !ShouldRegisterCustomFont(nullptr, false));
    {
        const char mgr = 1;
        expect_true("empty family skip register", !ShouldRegisterCustomFont(&mgr, true));
        expect_true("resMgr + family register", ShouldRegisterCustomFont(&mgr, false));
    }

    // leftover: caller skip RegisterCustomFont if !rootViewLock; still set families.
    expect_true("empty family no SetFontFamilies", !ShouldSetFontFamilies(true));
    expect_true("non-empty family SetFontFamilies", ShouldSetFontFamilies(false));
    expect_true("null rootViewLock skip RegisterCustomFont", !ShouldCallRegisterCustomFont(nullptr));
    {
        const char lock = 1;
        expect_true("rootViewLock calls RegisterCustomFont", ShouldCallRegisterCustomFont(&lock));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover custom-font rawfile null tests passed\n");
    return 0;
}
