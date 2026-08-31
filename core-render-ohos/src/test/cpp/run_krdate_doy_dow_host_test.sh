#!/usr/bin/env bash
# Compile + run KRDate leftover DAY_OF_YEAR / DAY_OF_WEEK host unit tests
# (no Harmony device).
#
# Usage:
#   ./run_krdate_doy_dow_host_test.sh          # g++ default
#   ./run_krdate_doy_dow_host_test.sh asan     # Address+UB sanitizers
#
# KRDate.cpp has no OHOS APIs. Host g++/clang++ is enough.
# TZ=UTC so the fixed-millis fixtures match localtime/mktime.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CALENDAR_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/expand/modules/calendar"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -I"$CALENDAR_DIR"
)

SRCS=(
    "$SCRIPT_DIR/krdate_doy_dow_host_test.cpp"
    "$CALENDAR_DIR/KRDate.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/krdate_doy_dow_host_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/krdate_doy_dow_host_test_asan"
        echo ">>> [$CXX] ASan/UBSan compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined "${SRCS[@]}" -o "$BIN"
        ;;
    *)
        echo "unknown mode: $MODE (default|asan)"
        exit 2
        ;;
esac

export TZ=UTC
echo ">>> run $BIN (TZ=$TZ)"
"$BIN"
