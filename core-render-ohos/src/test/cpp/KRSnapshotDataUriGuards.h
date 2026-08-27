#ifndef CORE_RENDER_OHOS_TEST_KRSNAPSHOT_DATAURI_GUARDS_H
#define CORE_RENDER_OHOS_TEST_KRSNAPSHOT_DATAURI_GUARDS_H

#include <cstddef>
#include <cstdint>
#include <string>

// Header-only leftover dataUri guards from KRSnapshotManager.cpp.
// Production cpp cannot host-compile (ArkUI / Harmony NDK).
//
// Leftover ProcessSnapshotResultWithDataType:
//   size = width * height * 4; malloc(size);
//   no overflow / zero / null check. PackToData / GetImageInfo on bad input.
// Leftover TakeSnapshot dataUri path:
//   drawableDescriptor was null-checked; pixelMap was not.
//
// Same saturate as ComputeDataUriPackedSize in KRSnapshotManager.cpp.

inline bool KRSnapshotDataUriComputePackedSize(uint32_t width, uint32_t height, size_t *out_size) {
    if (out_size == nullptr) {
        return false;
    }
    if (width == 0 || height == 0) {
        return false;
    }
    const size_t w = static_cast<size_t>(width);
    const size_t h = static_cast<size_t>(height);
    if (w > SIZE_MAX / h) {
        return false;
    }
    const size_t pixels = w * h;
    if (pixels > SIZE_MAX / 4) {
        return false;
    }
    *out_size = pixels * 4;
    return true;
}

// dataUri path: reject null/undefined pixelMap before ProcessSnapshotResultWithDataType.
inline bool KRSnapshotDataUriPixelMapRejected(bool is_null, bool is_undefined) {
    return is_null || is_undefined;
}

struct KRSnapshotDataUriResult {
    int code;
    std::string message;
};

inline KRSnapshotDataUriResult KRSnapshotDataUriRejectNullPixelMap() {
    KRSnapshotDataUriResult result;
    result.code = -1;
    result.message = "pixelMap is null or undefined";
    return result;
}

#endif  // CORE_RENDER_OHOS_TEST_KRSNAPSHOT_DATAURI_GUARDS_H
