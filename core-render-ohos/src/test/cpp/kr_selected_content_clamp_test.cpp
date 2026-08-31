// Host unit test for leftover KRRichTextView::GetSelectedContent start/end OOB.
//
// Leftover:
//   only clamps end; then
//     pre      = str16.substr(0, start)
//     selected = str16.substr(start, sel_end - start)
//   start > size  → std::out_of_range
//   start > end   → size_t underflow on (sel_end - start)
//
// Build + run (from this directory, no Harmony device):
//   ./run_kr_selected_content_clamp_test.sh
//   ./run_kr_selected_content_clamp_test.sh asan

#include "KRSelectedContentClamp.h"

#include <cstdio>
#include <stdexcept>
#include <string>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

// Mirror leftover GetSelectedContent: clamp end only, then substr.
static std::u16string leftover_selected(const std::u16string &str16, int start, int end) {
    size_t sel_end = static_cast<size_t>(end);
    if (sel_end > str16.size()) {
        sel_end = str16.size();
    }
    return str16.substr(static_cast<size_t>(start), sel_end - static_cast<size_t>(start));
}

static std::u16string leftover_pre(const std::u16string &str16, int start) {
    if (start > 0) {
        return str16.substr(0, static_cast<size_t>(start));
    }
    return {};
}

static void leftover_start_gt_length_throws(const char *name, const std::u16string &str16, int start, int end) {
    bool threw = false;
    try {
        (void)leftover_selected(str16, start, end);
    } catch (const std::out_of_range &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover substr threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
}

// leftover pre uses substr(0, start): pos is 0 so start>size does not throw
// (count is truncated). Clamp still keeps pre aligned with the selected range.
static void leftover_pre_start_gt_length_truncates(const char *name, const std::u16string &str16, int start) {
    std::u16string pre;
    try {
        pre = leftover_pre(str16, start);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover pre substr threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, pre == str16);
}

static void leftover_inverted_underflow(const char *name, int start, int end) {
    const size_t sel_start = static_cast<size_t>(start);
    size_t sel_end = static_cast<size_t>(end);
    const size_t wrapped = sel_end - sel_start;
    expect_true(name, start > end && wrapped > static_cast<size_t>(start));
}

static void expect_slice(const char *name, const std::u16string &str16, int start, int end,
                         const std::u16string &want_pre, const std::u16string &want_selected,
                         const std::u16string &want_post) {
    kuikly::util::KRSelectedContentParts parts;
    try {
        parts = kuikly::util::SliceSelectedUtf16Content(str16, start, end);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (parts.pre != want_pre || parts.selected != want_selected || parts.post != want_post) {
        std::fprintf(stderr, "FAIL %s: pre=%zu selected=%zu post=%zu\n", name, parts.pre.size(),
                     parts.selected.size(), parts.post.size());
        ++g_failed;
        return;
    }
    std::printf("PASS %s (selected=%zu, no throw)\n", name, parts.selected.size());
}

static void expect_range(const char *name, int start, int end, size_t size, size_t want_start, size_t want_end) {
    std::pair<size_t, size_t> range;
    try {
        range = kuikly::util::ClampSelectedUtf16Range(start, end, size);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: threw std::exception: %s\n", name, e.what());
        ++g_failed;
        return;
    } catch (...) {
        std::fprintf(stderr, "FAIL %s: threw unknown exception\n", name);
        ++g_failed;
        return;
    }
    if (range.first != want_start || range.second != want_end || range.first > range.second ||
        range.second > size) {
        std::fprintf(stderr, "FAIL %s: got [%zu, %zu) want [%zu, %zu) size=%zu\n", name, range.first,
                     range.second, want_start, want_end, size);
        ++g_failed;
        return;
    }
    std::printf("PASS %s ([%zu, %zu))\n", name, range.first, range.second);
}

int main() {
    const std::u16string hello = u"hello";  // size 5
    const std::u16string empty;

    // Document leftover abort / wrap polarity.
    leftover_start_gt_length_throws("leftover start>length throws", hello, 10, 12);
    leftover_pre_start_gt_length_truncates("leftover pre start>length truncates", hello, 10);
    leftover_inverted_underflow("leftover (5,3) size_t underflow", 5, 3);

    // Required leftover inputs: empty/clamped, no out_of_range.
    expect_range("clamp (5,3) size=5", 5, 3, 5, 3, 3);
    expect_slice("(5,3) on hello", hello, 5, 3, u"hel", u"", u"lo");
    expect_range("clamp start>length", 10, 12, 5, 5, 5);
    expect_slice("start>length on hello", hello, 10, 12, u"hello", u"", u"");
    expect_slice("pre path start>length", hello, 10, 3, u"hel", u"", u"lo");

    // Adjacent valid / edge ranges still slice.
    expect_slice("valid [1,4)", hello, 1, 4, u"h", u"ell", u"o");
    expect_slice("full [0,5)", hello, 0, 5, u"", u"hello", u"");
    expect_slice("empty [2,2)", hello, 2, 2, u"he", u"", u"llo");
    expect_slice("end>length", hello, 2, 99, u"he", u"llo", u"");
    expect_slice("negative start/end", hello, -1, -2, u"", u"", u"hello");
    expect_slice("empty string (5,3)", empty, 5, 3, u"", u"", u"");
    expect_slice("empty string start>length", empty, 1, 2, u"", u"", u"");

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
