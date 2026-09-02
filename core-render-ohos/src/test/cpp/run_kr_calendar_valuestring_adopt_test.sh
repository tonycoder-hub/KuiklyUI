#!/usr/bin/env bash
# Compile + run leftover KRCalendarModule valuestring adopt host tests
# (no Harmony device).
#
# Usage:
#   ./run_kr_calendar_valuestring_adopt_test.sh          # g++ default
#   ./run_kr_calendar_valuestring_adopt_test.sh asan     # Address+UB sanitizers
#
# The leftover adopt helper is NDK-free. Host g++/clang++ + bundled cJSON
# (compiled as C) is enough. Do not pass -std=c++17 to cJSON.c.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_DIR="$SCRIPT_DIR/../../main/cpp"
CALENDAR_DIR="$CPP_DIR/libohos_render/expand/modules/calendar"
CJSON_DIR="$CPP_DIR/thirdparty/cJSON"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
CC="${CC:-gcc}"
MODE="${1:-default}"

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -I"$CALENDAR_DIR"
    -I"$CPP_DIR"
)

C_FLAGS=(
    -Wall
    -Wextra
    -I"$CPP_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_calendar_valuestring_adopt_test.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_calendar_valuestring_adopt_test"
        CJSON_OBJ="$OUT_DIR/cJSON.o"
        echo ">>> [$CC] compile $CJSON_OBJ"
        "$CC" "${C_FLAGS[@]}" -O2 -c "$CJSON_DIR/cJSON.c" -o "$CJSON_OBJ"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" "$CJSON_OBJ" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_calendar_valuestring_adopt_test_asan"
        CJSON_OBJ="$OUT_DIR/cJSON_asan.o"
        echo ">>> [$CC] ASan/UBSan compile $CJSON_OBJ"
        "$CC" "${C_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined -c "$CJSON_DIR/cJSON.c" -o "$CJSON_OBJ"
        echo ">>> [$CXX] ASan/UBSan compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined "${SRCS[@]}" "$CJSON_OBJ" -o "$BIN"
        ;;
    *)
        echo "unknown mode: $MODE (default|asan)"
        exit 2
        ;;
esac

echo ">>> run $BIN"
"$BIN"
