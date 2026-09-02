#!/usr/bin/env bash
# Compile + run KREncodeURLComponent host unit tests (no Harmony device).
#
# Usage:
#   ./run_kr_codec_test.sh          # g++ default
#   ./run_kr_codec_test.sh asan     # Address+UB sanitizers
#
# KRCodec.cpp has no OHOS APIs. Host g++/clang++ is enough.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CODEC_DIR="$SCRIPT_DIR/../../main/cpp/libohos_render/expand/modules/codec"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"

CXX="${CXX:-g++}"
MODE="${1:-default}"

# Host g++ does not pull <vector> transitively the way the OHOS SDK does.
# Inject it only for this TU so the production codec stays drive-by-free.
HOST_COMPAT="$OUT_DIR/host_compat.h"
cat > "$HOST_COMPAT" <<'EOF'
#include <vector>
EOF

COMMON_FLAGS=(
    -std=c++17
    -Wall
    -Wextra
    -fsigned-char
    -include "$HOST_COMPAT"
    -I"$CODEC_DIR"
)

SRCS=(
    "$SCRIPT_DIR/kr_encode_url_component_test.cpp"
    "$CODEC_DIR/KRCodec.cpp"
    "$CODEC_DIR/md5.c"
    "$CODEC_DIR/sha256.c"
)

case "$MODE" in
    default)
        BIN="$OUT_DIR/kr_encode_url_component_test"
        echo ">>> [$CXX] compile $BIN"
        "$CXX" "${COMMON_FLAGS[@]}" -O2 "${SRCS[@]}" -o "$BIN"
        ;;
    asan)
        BIN="$OUT_DIR/kr_encode_url_component_test_asan"
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
