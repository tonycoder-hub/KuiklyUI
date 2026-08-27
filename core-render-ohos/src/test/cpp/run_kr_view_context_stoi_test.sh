#!/usr/bin/env bash
# Compile + run leftover KRViewContext constructor host tests
# (no Harmony device).
#
# Usage:
#   ./run_kr_view_context_stoi_test.sh          # g++ default
#   ./run_kr_view_context_stoi_test.sh asan     # Address+UB sanitizers
#
# KRViewContext.h is already header-only (no ArkUI). Host g++ includes it.

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
    "$SCRIPT_DIR/kr_view_context_stoi_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_view_context_stoi_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_view_context_stoi_test_asan"
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
