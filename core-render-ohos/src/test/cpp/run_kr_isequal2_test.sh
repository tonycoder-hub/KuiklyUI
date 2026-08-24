#!/usr/bin/env bash
# Compile + run leftover isEqual2 NULL==NULL host tests (no Harmony device).
#
# Usage:
#   ./run_kr_isequal2_test.sh          # g++ default
#   ./run_kr_isequal2_test.sh asan     # Address+UB sanitizers
#
# KRCstringEqual.h is header-only and has no OHOS APIs. Host g++/clang++
# includes it directly.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UTIL_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/utils"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$UTIL_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_isequal2_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_isequal2_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_isequal2_test_asan"
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
