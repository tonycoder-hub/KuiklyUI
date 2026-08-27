// Host unit test for leftover KRGradientRichTextShadow empty-color 0-size VLA.
//
// Build + run (from this directory, no Harmony device):
//   ./run_gradient_richtext_empty_color_test.sh
//   ./run_gradient_richtext_empty_color_test.sh asan

#include "gradient_richtext_empty_color_host.h"

#include <cstdio>

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
        std::vector<uint32_t> colors;
        std::vector<float> locations;
        expect_true("empty colors+locations skip shader",
                    ShouldSkipGradientShader(colors, locations));
    }
    {
        std::vector<uint32_t> colors;
        std::vector<float> locations{0.0f, 1.0f};
        expect_true("empty colors skip shader", ShouldSkipGradientShader(colors, locations));
    }
    {
        std::vector<uint32_t> colors{0xff0000u};
        std::vector<float> locations;
        expect_true("empty locations skip shader", ShouldSkipGradientShader(colors, locations));
    }
    {
        std::vector<uint32_t> colors{0xff0000u, 0x00ff00u};
        std::vector<float> locations{0.0f, 0.5f};
        expect_true("non-empty does not skip", !ShouldSkipGradientShader(colors, locations));
        std::vector<uint32_t> colors_array;
        std::vector<float> stops_array;
        BuildGradientArrays(colors, locations, colors_array, stops_array);
        expect_true("non-empty builds color array",
                    colors_array.size() == 2 && colors_array[0] == 0xff0000u &&
                        colors_array[1] == 0x00ff00u);
        expect_true("non-empty builds stops (last forced 1.0)",
                    stops_array.size() == 2 && stops_array[0] == 0.0f && stops_array[1] == 1.0f);
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all leftover gradient richtext empty-color tests passed\n");
    return 0;
}
