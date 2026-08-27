// Host unit test for leftover InsertSubRenderView root-path null-deref.
//
// Build + run (no Harmony):
//   ./run_insert_sub_render_view_test.sh
//   ./run_insert_sub_render_view_test.sh asan

#include "insert_sub_render_view_host.h"

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
        InsertHostMap registry;
        auto r = InsertSubRenderView(registry, -1, 7);
        expect_true("missing child_tag does not call AddContentView", !r.called_add_content);
        expect_true("missing child_tag does not insert", !r.called_insert_sub);
        expect_true("missing child_tag does not default-insert", registry.find(7) == registry.end());
    }
    {
        InsertHostMap registry;
        registry[3] = std::make_shared<InsertHostView>(InsertHostView{3});
        auto r = InsertSubRenderView(registry, -1, 3);
        expect_true("root with live child calls AddContentView", r.called_add_content && r.added && r.added->tag == 3);
    }
    {
        int dummy_root = 1;
        expect_true("AddContentView(nullptr) is no-op", !AddContentView(&dummy_root, nullptr));
        expect_true("AddContentView null root is no-op",
                    !AddContentView(nullptr, std::make_shared<InsertHostView>()));
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover insert sub render view tests passed\n");
    return 0;
}
