// Host leftover helper: custom-font OpenRawFile null / GetRawFileSize <= 0.
//
// Production KRParagraph.cpp LoadCustomFont and KRRichTextShadow.cpp
// RegisterCustomFont pull Harmony NDK (rawfile / drawing) and cannot
// host-compile. This header mirrors the FIXED rawfile load decision:
//   if (!rawFile) skip — do not GetRawFileSize / Read / Close, no alloc
//   if (len <= 0) skip — CloseRawFile if opened; do not allocate
//   leftover: GetRawFileSize -1 → unique_ptr<uint8_t[]>(size_t max) bad_alloc
//
// RegisterCustomFont also rejects null resMgr at start (KRParagraph L77).
// Caller still sets font families when !rootViewLock; skips RegisterCustomFont.

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace kuikly {
namespace leftover_custom_font {

enum class RawFileAction {
    SkipNullRawFile,  // OpenRawFile returned NULL
    SkipNonPositive,  // opened, GetRawFileSize <= 0
    Load,             // opened, len > 0 — allocate + read + close
};

struct RawFileLoadPlan {
    RawFileAction action;
    std::size_t planned_bytes;
    bool would_get_size;
    bool would_read;
    bool would_close;
    bool would_allocate;
};

// leftover polarity: make_unique<uint8_t[]>(len) with len == -1 is size_t max.
inline bool leftoverLenConvertsToSizeTMax(long len) {
    return static_cast<std::size_t>(len) == std::numeric_limits<std::size_t>::max();
}

// Mirror of the FIXED OpenRawFile / GetRawFileSize / allocate path.
// rawFile == nullptr means OpenRawFile failed (missing file or null resMgr).
// len is the GetRawFileSize result when a file was opened (ignored if null).
inline RawFileLoadPlan PlanRawFileLoad(const void *rawFile, long len) {
    RawFileLoadPlan p{};
    p.planned_bytes = 0;
    p.would_get_size = false;
    p.would_read = false;
    p.would_close = false;
    p.would_allocate = false;
    if (rawFile == nullptr) {
        p.action = RawFileAction::SkipNullRawFile;
        return p;
    }
    p.would_get_size = true;
    if (len <= 0) {
        p.action = RawFileAction::SkipNonPositive;
        p.would_close = true;
        return p;
    }
    p.action = RawFileAction::Load;
    p.planned_bytes = static_cast<std::size_t>(len);
    p.would_read = true;
    p.would_close = true;
    p.would_allocate = true;
    return p;
}

// RegisterCustomFont: reject null resMgr / empty family (KRParagraph L77).
inline bool ShouldRegisterCustomFont(const void *resMgr, bool fontFamilyEmpty) {
    return resMgr != nullptr && !fontFamilyEmpty;
}

// Caller: skip RegisterCustomFont if !rootViewLock; still set font families.
inline bool ShouldSetFontFamilies(bool fontFamilyEmpty) {
    return !fontFamilyEmpty;
}

inline bool ShouldCallRegisterCustomFont(const void *rootViewLock) {
    return rootViewLock != nullptr;
}

}  // namespace leftover_custom_font
}  // namespace kuikly
