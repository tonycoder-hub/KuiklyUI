#!/usr/bin/env bash
# Compile + run leftover ApngParser subBuffer/fdAT host unit tests (no Harmony device).
#
# Usage:
#   ./run_apng_parser_buffer_test.sh          # g++ default
#   ./run_apng_parser_buffer_test.sh asan     # Address+UB sanitizers
#
# ApngParser.h pulls APNGStructs.h (ArkUI). Helpers live in header-only
# ApngParserBuffer.h, so host g++/clang++ is enough.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APNG_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/expand/components/apng"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$APNG_DIR"
)

SRCS=(
    "$SCRIPT_DIR/apng_parser_buffer_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/apng_parser_buffer_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/apng_parser_buffer_test_asan"
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
