#!/usr/bin/env bash
# Compile + run leftover KRJSONObject GetString host unit tests (no Harmony device).
#
# Usage:
#   ./run_kr_json_object_getstring_test.sh          # g++ default
#   ./run_kr_json_object_getstring_test.sh asan     # Address+UB sanitizers
#
# KRJSONObject.cpp has no OHOS APIs. Host g++/clang++ + bundled cJSON is enough.
# The adopt/get-string helper can also wrap a tiny stub that returns nullptr
# for non-strings (see kr_json_object_getstring_test.cpp).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_DIR="$SCRIPT_DIR/../../main/cpp"
UTIL_DIR="$CPP_DIR/libohos_render/utils"
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
    -I"$UTIL_DIR"
    -I"$CPP_DIR"
)

C_FLAGS=(
    -Wall
    -Wextra
    -I"$CPP_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_json_object_getstring_test.cpp"
    "$UTIL_DIR/KRJSONObject.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_json_object_getstring_test"
        CJSON_OBJ="$OUT_DIR/cJSON.o"
        echo ">>> [$CC] compile $CJSON_OBJ"
        "$CC" "${C_FLAGS[@]}" -O2 -c "$CJSON_DIR/cJSON.c" -o "$CJSON_OBJ"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" "$CJSON_OBJ" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_json_object_getstring_test_asan"
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
