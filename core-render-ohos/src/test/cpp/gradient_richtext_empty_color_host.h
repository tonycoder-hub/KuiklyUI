#ifndef CORE_RENDER_OHOS_TEST_GRADIENT_RICHTEXT_EMPTY_COLOR_HOST_H
#define CORE_RENDER_OHOS_TEST_GRADIENT_RICHTEXT_EMPTY_COLOR_HOST_H

#include <cstdint>
#include <vector>

// Extracted leftover KRGradientRichTextShadow::DidBuildTextStyle
// (empty colors / locations after a successful parse).
//
// Leftover:
//   ParseFromCssLinearGradient can return true for linear-gradient(0)
//   / linear-gradient(0, #ff0000) with no color-stop pair, leaving
//   text_linearGradient_ non-null and GetColors()/GetLocations() empty.
//   uint32_t colorsArray[colors.size()] is then a 0-size VLA (UB) and
//   CreateLinearGradient(..., count=0).
//
// Production (no Drawing / Harmony in this helper):
//   empty colors or empty locations => skip shader (no VLA, no create)
//   non-empty => still build the C arrays
//
// Header-only so host leftover tests compile without ArkUI / Harmony.

inline bool ShouldSkipGradientShader(const std::vector<uint32_t> &colors,
                                     const std::vector<float> &locations) {
    return colors.empty() || locations.empty();
}

inline void BuildGradientArrays(const std::vector<uint32_t> &colors,
                                const std::vector<float> &locations,
                                std::vector<uint32_t> &colors_array,
                                std::vector<float> &stops_array) {
    colors_array.resize(colors.size());
    stops_array.resize(locations.size());
    for (size_t i = 0; i < colors.size(); ++i) {
        colors_array[i] = colors[i];
    }
    for (size_t i = 0; i < locations.size(); ++i) {
        if (i == locations.size() - 1) {
            stops_array[i] = 1.0f;
        } else {
            stops_array[i] = locations[i];
        }
    }
}

#endif  // CORE_RENDER_OHOS_TEST_GRADIENT_RICHTEXT_EMPTY_COLOR_HOST_H
