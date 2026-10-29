#!/usr/bin/env bash
# Generate reproducibility manifest for ATLAS OF PARTIES results
# Per spec section 8: SHA256 manifest with binary, compiler, flags, seeds, schedule

set -euo pipefail

OUT_DIR="${1:-results/atlas_train}"
PHASE="${2:-train}"
SEED_RANGE="${3:-1001..1100}"
SCHEDULE_FILE="${4:-tests/fixtures/burst.csv}"
BINARY_HASH="${5:-$(sha256sum ./proteusd 2>/dev/null | cut -d' ' -f1 || echo unknown)}"

mkdir -p "$OUT_DIR"
MANIFEST="$OUT_DIR/manifest.txt"

COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
CC_VERSION=$(cc --version 2>/dev/null | head -1 || echo "unknown")
CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -ffp-contract=off -D_DEFAULT_SOURCE"
NOW=$(date -Is)

SCHEDULE_HASH=$(sha256sum "$SCHEDULE_FILE" 2>/dev/null | cut -d' ' -f1 || echo "missing")

{
    echo "# Generated: $NOW"
    echo "# Binary: proteus $COMMIT"
    echo "# Binary SHA256: $BINARY_HASH"
    echo "# Compiler: $CC_VERSION"
    echo "# CFLAGS: $CFLAGS"
    echo "# Seed range: $SEED_RANGE"
    echo "# Duration: 600"
    echo "# Schedule: $SCHEDULE_FILE $SCHEDULE_HASH"
    echo ""
    # SHA256 of each output file
    for f in atlas_parties.csv atlas_moves.csv atlas_drivers.csv; do
        if [[ -f "$OUT_DIR/$f" ]]; then
            h=$(sha256sum "$OUT_DIR/$f" | cut -d' ' -f1)
            echo "$OUT_DIR/$f: $h"
        else
            echo "$OUT_DIR/$f: MISSING"
        fi
    done
} > "$MANIFEST"

echo "Manifest written: $MANIFEST"
cat "$MANIFEST"
