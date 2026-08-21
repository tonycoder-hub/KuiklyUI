#!/usr/bin/env bash
# Compile + run leftover getNApiArgsStdString adopt host tests (no Harmony device).
#
# Usage:
#   ./run_napi_args_stdstring_host_test.sh          # g++ default
#   ./run_napi_args_stdstring_host_test.sh asan     # Address+UB sanitizers
#
# adopt_napi_cstr is NAPI-free. Host g++/clang++ is enough.

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
    "$SCRIPT_DIR/napi_args_stdstring_host_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/napi_args_stdstring_host_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/napi_args_stdstring_host_test_asan"
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
