#!/usr/bin/env bash
# Compile + run the no-device KRThread worker-stack unit test.
#
# Usage:
#   ./run_krthread_stack.sh
#
# Applies ulimit -s 256 so a default pthread / old std::thread worker is
# ~256 KiB (OpenHarmony's ~132 KiB default is the same class of bug).
# KRSizedThread must still produce an 8 MiB stack.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/test_krthread_worker_stack.cpp"
KRTHREAD_CPP="$SCRIPT_DIR/../../main/cpp/libohos_render/foundation/thread/KRThread.cpp"
INCLUDE_DIR="$SCRIPT_DIR/../../main/cpp"
OUT_DIR="$SCRIPT_DIR/build"
mkdir -p "$OUT_DIR"
BIN="$OUT_DIR/test_krthread_worker_stack"

if [[ ! -f "$KRTHREAD_CPP" ]]; then
    echo "FAIL: missing $KRTHREAD_CPP"
    exit 1
fi

# KRThread.cpp should use the drop-in wrapper like std::thread.
if ! grep -q 'm_workerThread = KRSizedThread' "$KRTHREAD_CPP"; then
    echo "FAIL: KRThread.cpp does not construct the worker with KRSizedThread"
    exit 1
fi
if grep -E 'm_workerThread = std::thread' "$KRTHREAD_CPP"; then
    echo "FAIL: KRThread.cpp still constructs the worker with std::thread"
    exit 1
fi
# pthread stack size must stay isolated inside the wrapper.
if grep -E 'pthread_create|pthread_join|pthread_attr_setstacksize' "$KRTHREAD_CPP"; then
    echo "FAIL: KRThread.cpp still inlines pthread create/join/stack size"
    exit 1
fi

CXX="${CXX:-g++}"
echo ">>> compile $BIN with $CXX"
"$CXX" -std=c++17 -O0 -g -Wall -Wextra -Werror -pthread \
    -D_GNU_SOURCE \
    -I "$INCLUDE_DIR" \
    "$SRC" -o "$BIN"

# 256 KiB default stack: small enough that the pre-fix std::thread worker fails.
ulimit -s 256

echo ">>> ulimit -s $(ulimit -s)"
echo ">>> run $BIN"
"$BIN"
