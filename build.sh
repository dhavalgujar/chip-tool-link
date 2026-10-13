#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0

# Builds chip-tool-link against a connectedhomeip checkout, without changing
# the checkout: chip-tool's sources compile as they are.
#
# Usage: build.sh CHIP_ROOT [OUT_DIR]
#   CHIP_ROOT  a connectedhomeip checkout with its submodules, bootstrapped
#              (tested at 7d3dfb6c06b3cc696186f86d527b2f387a5603df)
#   OUT_DIR    default ./out (relative to where it was run)
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
CHIP=$(cd "${1:?usage: build.sh CHIP_ROOT [OUT_DIR]}" && pwd)
OUT=${2:-out}
case $OUT in /*) ;; *) OUT=$PWD/$OUT ;; esac

# The GN root's links: CHIP's build overrides and CHIP itself.
mkdir -p "$HERE/third_party"
ln -sfn "$CHIP/examples/build_overrides" "$HERE/build_overrides"
ln -sfn "$CHIP" "$HERE/third_party/connectedhomeip"

cd "$CHIP"
if ! command -v gn > /dev/null; then
    set +u
    . scripts/activate.sh > /dev/null
    set -u
fi
command -v gn > /dev/null || { echo "build.sh: no gn: set up CHIP's environment first (scripts/bootstrap.sh)" >&2; exit 1; }
gn gen --root="$HERE" "$OUT"
ninja -C "$OUT" chip-tool-link
ls -l "$OUT/chip-tool-link"
