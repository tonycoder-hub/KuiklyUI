// Host unit test for leftover DAY_OF_YEAR / DAY_OF_WEEK set/add no-ops in KRDate.
//
// POSIX mktime ignores tm_yday and tm_wday. Writing only those fields left
// CalDate DAY_OF_YEAR / DAY_OF_WEEK set/add as no-ops vs Android Calendar / Web.
//
// KRDate.cpp has no OHOS APIs, so this can be built with host g++/clang++.
// Run with TZ=UTC (see run_krdate_doy_dow_host_test.sh) so localtime/mktime
// match the fixed UTC millis below.
//
// Build + run (from this directory):
//   ./run_krdate_doy_dow_host_test.sh
//   ./run_krdate_doy_dow_host_test.sh asan

#include "KRDate.h"

#include <cstdint>
#include <cstdio>

using kuikly::util::Date;

static int g_failed = 0;

static void expect_eq(const char *name, std::int64_t got, std::int64_t want) {
    if (got != want) {
        std::fprintf(stderr, "FAIL %s: got %lld want %lld\n", name,
                     static_cast<long long>(got), static_cast<long long>(want));
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static void expect_ne(const char *name, std::int64_t got, std::int64_t notWant) {
    if (got == notWant) {
        std::fprintf(stderr, "FAIL %s: still %lld (no-op)\n", name,
                     static_cast<long long>(got));
        ++g_failed;
    } else {
        std::printf("PASS %s (changed from %lld to %lld)\n", name,
                    static_cast<long long>(notWant), static_cast<long long>(got));
    }
}

int main() {
    // 2024-01-15T12:00:00.000Z — Monday, day-of-year 15, leap year.
    // Sunday=1 … Saturday=7, so Monday GetDateOfWeek() == 2.
    const std::int64_t kFixedMillis = 1705320000000LL;
    const int kMonday = 2;
    const int kDayOfYearJan15 = 15;

    {
        Date baseline(kFixedMillis);
        expect_eq("fixed millis GetTime()", baseline.GetTime(), kFixedMillis);
        expect_eq("fixed millis year", baseline.GetFullYear(), 2024);
        expect_eq("fixed millis month", baseline.GetMonth(), 0);
        expect_eq("fixed millis day", baseline.GetDate(), 15);
        expect_eq("fixed millis dayOfYear", baseline.GetDateOfYear(), kDayOfYearJan15);
        expect_eq("fixed millis dayOfWeek", baseline.GetDateOfWeek(), kMonday);
        expect_eq("fixed millis hours", baseline.GetHours(), 12);
    }

    // DAY_OF_YEAR set: Web-style setFullYear(year, 0, 200) → 2024-07-18 (leap).
    {
        Date d(kFixedMillis);
        std::int64_t before = d.GetTime();
        d.SetDateOfYear(200);
        std::int64_t after = d.GetTime();
        expect_ne("SetDateOfYear(200) GetTime()", after, before);
        // GetField path: GetTime() normalizes, then fields must match the new day.
        expect_eq("SetDateOfYear(200) GetDateOfYear after GetTime", d.GetDateOfYear(), 200);
        expect_eq("SetDateOfYear(200) month (July)", d.GetMonth(), 6);
        expect_eq("SetDateOfYear(200) day", d.GetDate(), 18);
        expect_eq("SetDateOfYear(200) year", d.GetFullYear(), 2024);
        expect_eq("SetDateOfYear(200) hours preserved", d.GetHours(), 12);
    }

    // DAY_OF_YEAR add: CalDate add uses SetDateOfYear(original + delta).
    {
        Date d(kFixedMillis);
        int original = d.GetDateOfYear();
        std::int64_t before = d.GetTime();
        d.SetDateOfYear(original + 10);
        std::int64_t after = d.GetTime();
        expect_ne("DAY_OF_YEAR add +10 GetTime()", after, before);
        expect_eq("DAY_OF_YEAR add +10 GetDateOfYear after GetTime", d.GetDateOfYear(),
                  original + 10);
        expect_eq("DAY_OF_YEAR add +10 day (Jan 25)", d.GetDate(), 25);
    }

    // DAY_OF_WEEK set: Monday(2) → Sunday(1) is -1 day (Android/Web week, Sun=1).
    {
        Date d(kFixedMillis);
        std::int64_t before = d.GetTime();
        d.SetDateOfWeek(1);
        std::int64_t after = d.GetTime();
        expect_ne("SetDateOfWeek(Sunday) GetTime()", after, before);
        expect_eq("SetDateOfWeek(Sunday) GetDateOfWeek after GetTime", d.GetDateOfWeek(), 1);
        expect_eq("SetDateOfWeek(Sunday) day (Jan 14)", d.GetDate(), 14);
        expect_eq("SetDateOfWeek(Sunday) millis delta", after - before, -86400000LL);
    }

    // DAY_OF_WEEK set: Monday(2) → Saturday(7) is +5 days.
    {
        Date d(kFixedMillis);
        std::int64_t before = d.GetTime();
        d.SetDateOfWeek(7);
        std::int64_t after = d.GetTime();
        expect_ne("SetDateOfWeek(Saturday) GetTime()", after, before);
        expect_eq("SetDateOfWeek(Saturday) GetDateOfWeek after GetTime", d.GetDateOfWeek(), 7);
        expect_eq("SetDateOfWeek(Saturday) day (Jan 20)", d.GetDate(), 20);
        expect_eq("SetDateOfWeek(Saturday) millis delta", after - before, 5 * 86400000LL);
    }

    // DAY_OF_WEEK add: CalDate add uses SetDateOfWeek(original + delta).
    {
        Date d(kFixedMillis);
        int original = d.GetDateOfWeek();
        std::int64_t before = d.GetTime();
        d.SetDateOfWeek(original + 3);
        std::int64_t after = d.GetTime();
        expect_ne("DAY_OF_WEEK add +3 GetTime()", after, before);
        expect_eq("DAY_OF_WEEK add +3 GetDateOfWeek after GetTime", d.GetDateOfWeek(), 5);
        expect_eq("DAY_OF_WEEK add +3 day (Jan 18)", d.GetDate(), 18);
        expect_eq("DAY_OF_WEEK add +3 millis delta", after - before, 3 * 86400000LL);
    }

    // Same-value set must keep GetTime() (not a random jump) and fields stable.
    {
        Date d(kFixedMillis);
        d.SetDateOfYear(kDayOfYearJan15);
        expect_eq("SetDateOfYear(same) GetTime()", d.GetTime(), kFixedMillis);
        expect_eq("SetDateOfYear(same) GetDateOfYear", d.GetDateOfYear(), kDayOfYearJan15);
    }
    {
        Date d(kFixedMillis);
        d.SetDateOfWeek(kMonday);
        expect_eq("SetDateOfWeek(same) GetTime()", d.GetTime(), kFixedMillis);
        expect_eq("SetDateOfWeek(same) GetDateOfWeek", d.GetDateOfWeek(), kMonday);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
