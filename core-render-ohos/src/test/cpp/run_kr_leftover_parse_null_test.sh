#!/usr/bin/env bash
# Compile + run leftover JSONObject::Parse-null host unit tests (no Harmony device).
#
# Usage:
#   ./run_kr_leftover_parse_null_test.sh          # g++ default
#   ./run_kr_leftover_parse_null_test.sh asan     # Address+UB sanitizers
#
# BackPress / Scroller / OH prefs pull in NAPI. Thin wrappers in the
# test mimic those Parse+use paths. KRPageCreateTrace.cpp has no OHOS
# APIs and is compiled for real. KRJSONObject.cpp has no OHOS APIs, so
# host g++/clang++ + bundled cJSON is enough.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPP_DIR="$SCRIPT_DIR/../../main/cpp"
UTIL_DIR="$CPP_DIR/libohos_render/utils"
TRACE_DIR="$CPP_DIR/libohos_render/expand/modules/performance"
CJSON_DIR="$CPP_DIR/thirdparty/cJSON"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
CC="${CC:-gcc}"
MODE="${1:-default}"

# KRJSONObject.h uses std::shared_ptr / std::vector without including
# <memory>/<vector> (fixed in leftover #1651). KRPageCreateTrace.h uses
# int64_t without <cstdint>. Force-include so this leftover host test
# compiles without touching those files.
COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -include memory
    -include vector
    -include cstdint
    -I"$TRACE_DIR"
    -I"$UTIL_DIR"
    -I"$CPP_DIR"
)

C_FLAGS=(
    -Wall
    -Wextra
    -I"$CPP_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_leftover_parse_null_test.cpp"
    "$UTIL_DIR/KRJSONObject.cpp"
    "$TRACE_DIR/KRPageCreateTrace.cpp"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_leftover_parse_null_test"
        CJSON_OBJ="$OUT_DIR/cJSON.o"
        echo ">>> [$CC] compile $CJSON_OBJ"
        "$CC" "${C_FLAGS[@]}" -O2 -c "$CJSON_DIR/cJSON.c" -o "$CJSON_OBJ"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" "$CJSON_OBJ" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_leftover_parse_null_test_asan"
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
