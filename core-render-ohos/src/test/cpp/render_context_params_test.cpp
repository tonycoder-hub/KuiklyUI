// Host unit test for leftover KRRenderContextParams map[] / end()-deref.
//
// Build + run (no Harmony):
//   ./run_render_context_params_test.sh
//   ./run_render_context_params_test.sh asan

#include "render_context_params_host.h"

#include <cstdio>
#include <memory>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

int main() {
    {
        HostMap empty;
        auto r = InitFromPageData(nullptr, empty);
        expect_true("null page_data uses default mode", r.used_default_mode);
        expect_true("null page_data empty contextCode", r.context_code.empty());
        expect_true("null page_data PageParam is null", PageParam(nullptr, empty) == nullptr);
    }
    {
        auto page = std::make_shared<HostValue>();
        HostMap empty;
        auto r = InitFromPageData(page, empty);
        expect_true("missing keys uses default mode", r.used_default_mode);
        expect_true("missing contextCode is empty", r.context_code.empty());
        expect_true("missing PageParam is null", PageParam(page, empty) == nullptr);
    }
    {
        auto page = std::make_shared<HostValue>();
        HostMap map;
        map["executeMode"] = std::make_shared<HostValue>(HostValue{2, ""});
        map["contextCode"] = std::make_shared<HostValue>(HostValue{0, "abc"});
        map["param"] = std::make_shared<HostValue>(HostValue{0, "p"});
        auto r = InitFromPageData(page, map);
        expect_true("present executeMode is used", !r.used_default_mode && r.execute_mode == 2);
        expect_true("present contextCode is used", r.context_code == "abc");
        auto param = PageParam(page, map);
        expect_true("present PageParam returns value", param && param->toString() == "p");
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover render context params tests passed\n");
    return 0;
}
