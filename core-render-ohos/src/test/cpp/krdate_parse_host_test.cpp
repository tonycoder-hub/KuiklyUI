// Host unit test for leftover Date::Parse stoi / substr out-of-range.
//
// Leftover:
//   pos = formatStr.find(token);
//   pos -= quoteCount(formatStr.substr(0, pos));
//   std::stoi(dateStr.substr(pos, N));
//
// No check that pos+N <= dateStr.size(); non-numeric tokens throw.
// "xxxx" vs "yyyy" → std::invalid_argument
// "1" vs "yyyy-MM-dd" → std::out_of_range
//
// KRDate.cpp / KRDateParse.h have no OHOS APIs, so this builds with host g++.
//
// Build + run (from this directory):
//   ./run_krdate_parse_host_test.sh
//   ./run_krdate_parse_host_test.sh asan

#include "KRDate.h"
#include "KRDateParse.h"

#include <cstdio>
#include <stdexcept>
#include <string>

using kuikly::util::Date;
using kuikly::util::TryParseDateField;

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

static void leftover_stoi_throws(const char *name, const std::string &s) {
    bool threw = false;
    try {
        (void)std::stoi(s);
    } catch (const std::invalid_argument &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover stoi threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
}

static void leftover_substr_throws(const char *name, const std::string &s, std::string::size_type pos,
                                   std::string::size_type n) {
    bool threw = false;
    try {
        (void)s.substr(pos, n);
    } catch (const std::out_of_range &) {
        threw = true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: leftover substr threw %s\n", name, e.what());
        ++g_failed;
        return;
    }
    expect_true(name, threw);
}

int main() {
    leftover_stoi_throws("leftover stoi(\"xxxx\")", "xxxx");
    leftover_substr_throws("leftover substr(\"1\", 5, 2) for MM in yyyy-MM-dd", "1", 5, 2);

    // Date::Parse("xxxx","yyyy") does not throw; year field is skipped.
    {
        Date d;
        d.SetFullYear(1999);
        d.SetMonth(5);
        d.SetDate(15);
        std::string dateStr = "xxxx";
        std::string formatStr = "yyyy";
        bool threw = false;
        try {
            d.Parse(dateStr, formatStr);
        } catch (const std::exception &e) {
            std::fprintf(stderr, "FAIL Parse(\"xxxx\",\"yyyy\") threw: %s\n", e.what());
            threw = true;
            ++g_failed;
        } catch (...) {
            std::fprintf(stderr, "FAIL Parse(\"xxxx\",\"yyyy\") threw unknown\n");
            threw = true;
            ++g_failed;
        }
        expect_true("Date::Parse(\"xxxx\",\"yyyy\") does not throw", !threw);
        expect_eq("Parse(\"xxxx\",\"yyyy\") year unchanged", d.GetFullYear(), 1999);
        expect_eq("Parse(\"xxxx\",\"yyyy\") month unchanged", d.GetMonth(), 5);
        expect_eq("Parse(\"xxxx\",\"yyyy\") date unchanged", d.GetDate(), 15);
    }

    // Parse("1","yyyy-MM-dd") does not throw; every token is short / OOR.
    {
        Date d;
        d.SetFullYear(1999);
        d.SetMonth(5);
        d.SetDate(15);
        std::string dateStr = "1";
        std::string formatStr = "yyyy-MM-dd";
        bool threw = false;
        try {
            d.Parse(dateStr, formatStr);
        } catch (const std::exception &e) {
            std::fprintf(stderr, "FAIL Parse(\"1\",\"yyyy-MM-dd\") threw: %s\n", e.what());
            threw = true;
            ++g_failed;
        } catch (...) {
            std::fprintf(stderr, "FAIL Parse(\"1\",\"yyyy-MM-dd\") threw unknown\n");
            threw = true;
            ++g_failed;
        }
        expect_true("Date::Parse(\"1\",\"yyyy-MM-dd\") does not throw", !threw);
        expect_eq("Parse(\"1\",\"yyyy-MM-dd\") year unchanged", d.GetFullYear(), 1999);
        expect_eq("Parse(\"1\",\"yyyy-MM-dd\") month unchanged", d.GetMonth(), 5);
        expect_eq("Parse(\"1\",\"yyyy-MM-dd\") date unchanged", d.GetDate(), 15);
    }

    // Parse("2024-08-24","yyyy-MM-dd") sets year=2024 month=7 (0-based) date=24.
    {
        Date d;
        std::string dateStr = "2024-08-24";
        std::string formatStr = "yyyy-MM-dd";
        bool threw = false;
        try {
            d.Parse(dateStr, formatStr);
        } catch (const std::exception &e) {
            std::fprintf(stderr, "FAIL Parse(\"2024-08-24\",\"yyyy-MM-dd\") threw: %s\n", e.what());
            threw = true;
            ++g_failed;
        } catch (...) {
            std::fprintf(stderr, "FAIL Parse(\"2024-08-24\",\"yyyy-MM-dd\") threw unknown\n");
            threw = true;
            ++g_failed;
        }
        expect_true("Date::Parse(\"2024-08-24\",\"yyyy-MM-dd\") does not throw", !threw);
        expect_eq("Parse(\"2024-08-24\",\"yyyy-MM-dd\") year", d.GetFullYear(), 2024);
        expect_eq("Parse(\"2024-08-24\",\"yyyy-MM-dd\") month (0-based)", d.GetMonth(), 7);
        expect_eq("Parse(\"2024-08-24\",\"yyyy-MM-dd\") date", d.GetDate(), 24);
    }

    // Header helper: quoted prefix still maps onto the quote-stripped date.
    {
        int year = 0;
        bool ok = TryParseDateField("at 2024", "'at' yyyy", "yyyy", 4, year);
        expect_true("TryParseDateField quoted prefix", ok);
        expect_eq("TryParseDateField quoted prefix year", year, 2024);
    }

    // Header helper: quoteCount > pos would wrap; skip the field.
    // (Defensive — DateParseQuoteCount of a prefix cannot exceed its length.)
    {
        int out = 42;
        // Empty date + token at pos 0 is the pos < size / pos+width path.
        bool ok = TryParseDateField("", "yyyy", "yyyy", 4, out);
        expect_true("TryParseDateField empty date skips", !ok);
        expect_eq("TryParseDateField empty date leaves out", out, 42);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
