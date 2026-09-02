// Host unit test for KRDate leftover-negative / leftover-positive milliseconds.
//
// KRDate.cpp has no OHOS APIs, so this can be built with host g++/clang++.
// C++ / and % are toward-zero; JS/Android Date keep millis in [0, 999].
//
// Build + run (from this directory):
//   ./run_krdate_millis_host_test.sh
//   ./run_krdate_millis_host_test.sh asan

#include "KRDate.h"

#include <cstdio>

using kuikly::util::Date;

static int g_failed = 0;

static void expect_eq(const char *name, int got, int want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    // leftover-negative timestamp: Date(-1500) is 1.5s before epoch.
    // Toward-zero leftover: millis = -500. Floor-div: -2s + 500ms.
    expect_eq("Date(-1500).GetMilliseconds()", Date(-1500).GetMilliseconds(), 500);

    // leftover-negative exact second boundary stays 0 (no leftover remainder).
    expect_eq("Date(-1000).GetMilliseconds()", Date(-1000).GetMilliseconds(), 0);

    // leftover-negative just past a second: toward-zero leftover millis = -1.
    expect_eq("Date(-1001).GetMilliseconds()", Date(-1001).GetMilliseconds(), 999);

    // leftover-positive path unchanged.
    expect_eq("Date(1500).GetMilliseconds()", Date(1500).GetMilliseconds(), 500);
    expect_eq("Date(0).GetMilliseconds()", Date(0).GetMilliseconds(), 0);
    expect_eq("Date(999).GetMilliseconds()", Date(999).GetMilliseconds(), 999);
    expect_eq("Date(1000).GetMilliseconds()", Date(1000).GetMilliseconds(), 0);

    // SetMilliseconds leftover-negative: borrow one second, millis = 999.
    {
        Date d;
        int secBefore = d.GetSeconds();
        d.SetMilliseconds(-1);
        expect_eq("SetMilliseconds(-1).GetMilliseconds()", d.GetMilliseconds(), 999);
        expect_eq("SetMilliseconds(-1) seconds borrowed", d.GetSeconds(), secBefore - 1);
    }

    // SetMilliseconds leftover-positive carry and in-range value.
    {
        Date d;
        int secBefore = d.GetSeconds();
        d.SetMilliseconds(1500);
        expect_eq("SetMilliseconds(1500).GetMilliseconds()", d.GetMilliseconds(), 500);
        expect_eq("SetMilliseconds(1500) seconds carried", d.GetSeconds(), secBefore + 1);
    }
    {
        Date d;
        int secBefore = d.GetSeconds();
        d.SetMilliseconds(500);
        expect_eq("SetMilliseconds(500).GetMilliseconds()", d.GetMilliseconds(), 500);
        expect_eq("SetMilliseconds(500) seconds unchanged", d.GetSeconds(), secBefore);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
