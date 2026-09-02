#!/usr/bin/env bash
# Compile + run leftover Color::FromString host tests (no Harmony device).
#
# Usage:
#   ./run_kr_color_from_string_test.sh          # g++ default
#   ./run_kr_color_from_string_test.sh asan     # Address+UB sanitizers
#
# KRColor.h is header-only. Host g++/clang++ includes it directly. Production
# KRStringUtil.h pulls Harmony NDK headers, so the test includes a host stub
# first (same include guard) that implements only SplitString (rgba path).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UTIL_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/utils"
STUB_DIR="$SCRIPT_DIR/host_stubs"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$STUB_DIR"
    -I"$UTIL_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_color_from_string_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_color_from_string_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_color_from_string_test_asan"
        echo ">>> [$CXX] ASan/UBSan compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined "${SRCS[@]}" -o "$BIN"
        ;;
    *)
        echo "unknown mode: $MODE (default|asan)"
        exit 2
        ;;
esac

echo ">>> run $BIN"
"$BIN"
