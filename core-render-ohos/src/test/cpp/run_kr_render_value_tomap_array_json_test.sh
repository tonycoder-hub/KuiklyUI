#!/usr/bin/env bash
# Compile + run leftover KRRenderValue::toMap array-JSON host tests
# (no Harmony device).
#
# Usage:
#   ./run_kr_render_value_tomap_array_json_test.sh          # g++ default
#   ./run_kr_render_value_tomap_array_json_test.sh asan     # Address+UB sanitizers
#
# KRRenderValue.h cannot host-compile without Harmony. Helper
# KRRenderValueToMapHost.h + vendored cJSON.c / cJSON.h.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CJSON_DIR="$SCRIPT_DIR/../../main/cpp/thirdparty/cJSON"
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
    -I"$SCRIPT_DIR"
    -I"$CJSON_DIR"
)

CJSON_SRC="$CJSON_DIR/cJSON.c"
TEST_SRC="$SCRIPT_DIR/kr_render_value_tomap_array_json_test.cpp"

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_render_value_tomap_array_json_test"
        CJSON_OBJ="$OUT_DIR/cJSON.o"
        echo ">>> [$CC] compile $CJSON_OBJ"
        "$CC" -std=c11 -O2 -I"$CJSON_DIR" -c "$CJSON_SRC" -o "$CJSON_OBJ"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "$TEST_SRC" "$CJSON_OBJ" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_render_value_tomap_array_json_test_asan"
        CJSON_OBJ="$OUT_DIR/cJSON_asan.o"
        echo ">>> [$CC] ASan/UBSan compile $CJSON_OBJ"
        "$CC" -std=c11 -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined -I"$CJSON_DIR" -c "$CJSON_SRC" -o "$CJSON_OBJ"
        echo ">>> [$CXX] ASan/UBSan compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O1 -g -fno-omit-frame-pointer \
            -fsanitize=address,undefined "$TEST_SRC" "$CJSON_OBJ" -o "$BIN"
        ;;
    *)
        echo "unknown mode: $MODE (default|asan)"
        exit 2
        ;;
esac

echo ">>> run $BIN"
"$BIN"
