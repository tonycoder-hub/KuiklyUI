#!/usr/bin/env bash
# Host leftover: KRSnapshotModule FILE/cacheKey toFile catch contract.
# No Harmony device. Requires node.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
exec node "${DIR}/kr_snapshot_module_tofile_catch_test.js"
