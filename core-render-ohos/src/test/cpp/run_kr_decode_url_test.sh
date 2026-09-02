#!/usr/bin/env bash
# Compile + run leftover KRDecodeURLComponent host unit tests (no Harmony device).
#
# Usage:
#   ./run_kr_decode_url_test.sh          # g++ default
#   ./run_kr_decode_url_test.sh asan     # Address+UB sanitizers
#
# KRCodecDecode.h is Harmony-free. Host g++/clang++ is enough.
# -fsigned-char matches OHOS aarch64/x86_64 so leftover raw-char ctype is exercised.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CODEC_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/expand/modules/codec"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -fsigned-char
    -I"$CODEC_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_decode_url_component_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_decode_url_component_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_decode_url_component_test_asan"
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
