#!/usr/bin/env bash
# Compile + run leftover ConvertSplit/SplitString OOB host unit tests
# (no Harmony device).
#
# Usage:
#   ./run_kr_split_tokens_oob_test.sh          # g++ default
#   ./run_kr_split_tokens_oob_test.sh asan     # Address+UB sanitizers
#
# KRSplitTokens.h is header-only std C++ (plus KRBorderRadiuses.h).
# Host g++/clang++ is enough — no ArkUI.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_ROOT="$SCRIPT_DIR/../../main/cpp"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$CPP_ROOT"
)

SRCS=(
    "$SCRIPT_DIR/kr_split_tokens_oob_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_split_tokens_oob_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_split_tokens_oob_test_asan"
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
