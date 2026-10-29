#!/usr/bin/env bash
# Generate reproducibility manifest for G1-G7 hypothesis results
# Per reproducibility-manifest skill: SHA256 manifest with binary, compiler, flags, seeds, schedule

set -euo pipefail

OUT_DIR="${1:-results/hypotheses}"
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

# Also hash the observer tools
TOOLS_HASH=$(sha256sum tools/observer_passive.py tools/observer_active.py tools/observer_reconstruction.py tools/learnability.py 2>/dev/null | sha256sum | cut -d' ' -f1 || echo "missing")

{
    echo "# Generated: $NOW"
    echo "# Phase: $PHASE"
    echo "# Binary: proteus $COMMIT"
    echo "# Binary SHA256: $BINARY_HASH"
    echo "# Tools SHA256: $TOOLS_HASH"
    echo "# Compiler: $CC_VERSION"
    echo "# CFLAGS: $CFLAGS"
    echo "# Seed range: $SEED_RANGE"
    echo "# Duration: 600"
    echo "# Schedule: $SCHEDULE_FILE $SCHEDULE_HASH"
    echo ""
    # SHA256 of each output file
    for f in g1_g7_results.csv g1_results.json g2_results.json g3_results.json g4_results.json g5_results.json g6_results.json g7_results.json; do
        if [[ -f "$OUT_DIR/$f" ]]; then
            h=$(sha256sum "$OUT_DIR/$f" | cut -d' ' -f1)
            echo "$OUT_DIR/$f: $h"
        else
            echo "$OUT_DIR/$f: MISSING"
        fi
    done
    # Also include per-seed journal hashes if available
    if [[ -d "results/atlas_journals" ]]; then
        echo ""
        echo "# Journal hashes (first 10):"
        for f in results/atlas_journals/seed_*.csv; do
            [[ -f "$f" ]] || continue
            h=$(sha256sum "$f" | cut -d' ' -f1)
            echo "$f: $h"
        done | head -10
    fi
} > "$MANIFEST"

echo "Manifest written: $MANIFEST"
cat "$MANIFEST"
