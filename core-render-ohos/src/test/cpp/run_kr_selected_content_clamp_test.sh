#!/usr/bin/env bash
# Compile + run leftover GetSelectedContent start/end clamp host tests
# (no Harmony device).
#
# Usage:
#   ./run_kr_selected_content_clamp_test.sh          # g++ default
#   ./run_kr_selected_content_clamp_test.sh asan     # Address+UB sanitizers
#
# Clamp helper lives in header-only KRSelectedContentClamp.h (no ArkUI).
# Host g++/clang++ includes it directly.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RICHTEXT_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/expand/components/richtext"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$RICHTEXT_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_selected_content_clamp_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_selected_content_clamp_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_selected_content_clamp_test_asan"
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
