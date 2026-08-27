#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"
CXX="${CXX:-g++}"
MODE="${1:-default}"
COMMON_FLAGS=(-std=c++17 -Wall -Wextra -Werror -I"$SCRIPT_DIR")
SRCS=("$SCRIPT_DIR/render_context_params_test.cpp")
case "$MODE" in
    default)
        BIN="$OUT_DIR/render_context_params_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/render_context_params_test_asan"
        echo ">>> [$CXX] ASan/UBSan compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined "${SRCS[@]}" -o "$BIN"
        ;;
    *) echo "unknown mode: $MODE"; exit 2 ;;
esac
echo ">>> run $BIN"
"$BIN"
