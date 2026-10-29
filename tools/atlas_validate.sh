#!/usr/bin/env bash
# ATLAS OF PARTIES held-out validation — Phase 2 (r3)
# Runs on held-out seeds 2001..2100, produces:
#   results/atlas_validate/atlas_parties.csv
#   results/atlas_validate/atlas_moves.csv
#   results/atlas_validate/atlas_drivers.csv
# STRICT SEPARATION: forbidden from reading training journals or using training thresholds

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Configuration (frozen per spec)
SEED_START=2001
SEED_END=2100
SCHEDULE_FILE="tests/fixtures/burst.csv"
JOURNALS_DIR="results/atlas_journals"
OUT_DIR="results/atlas_validate"
DURATION_S=600
MAX_HOURS=24

mkdir -p "$OUT_DIR" "$JOURNALS_DIR"

# STRICT CHECK: refuse to run if training outputs exist in validate dir
if [[ -f "$OUT_DIR/atlas_parties.csv" ]]; then
    echo "ERROR: Validation output already exists. Remove manually if intentional." >&2
    exit 1
fi

# STRICT CHECK: refuse if training journals dir is used as validate journals
# (enforce separate journal storage per spec section 6.2)
VALIDATE_JOURNALS_DIR="results/atlas_journals_validate"
mkdir -p "$VALIDATE_JOURNALS_DIR"

# Validate
if [[ ! -f "$SCHEDULE_FILE" ]]; then
    echo "Schedule not found: $SCHEDULE_FILE" >&2
    exit 1
fi
if [[ ! -x ./proteusd ]]; then
    echo "proteusd not built. Run 'make'." >&2
    exit 1
fi

BINARY_HASH=$(sha256sum ./proteusd | cut -d' ' -f1)
SCHEDULE_HASH=$(sha256sum "$SCHEDULE_FILE" | cut -d' ' -f1)
COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
START_TIME=$(date -Is)

echo "=== ATLAS VALIDATE START (HELD-OUT) ==="
echo "Commit: $COMMIT"
echo "Binary: $BINARY_HASH"
echo "Schedule: $SCHEDULE_HASH"
echo "Seeds: $SEED_START..$SEED_END (HELD-OUT SET)"
echo "Output: $OUT_DIR"
echo "Journals: $VALIDATE_JOURNALS_DIR (separate from training)"

# Step 1: Run parties on held-out seeds
CHECKPOINT="$OUT_DIR/validate_checkpoint.txt"
./tools/atlas_runner.sh \
    --seeds "$SEED_START..$SEED_END" \
    --schedule "$SCHEDULE_FILE" \
    --out-dir "$VALIDATE_JOURNALS_DIR" \
    --max-hours "$MAX_HOURS" \
    --checkpoint "$CHECKPOINT" \
    --resume \
    --duration "$DURATION_S"

# Step 2: Analyze each journal
echo "=== ANALYZING HELD-OUT JOURNALS ==="
PARTIES_CSV="$OUT_DIR/atlas_parties.csv"
MOVES_CSV="$OUT_DIR/atlas_moves.csv"
DRIVERS_CSV="$OUT_DIR/atlas_drivers.csv"

# Initialize CSV files with headers
python3 -c "
import pandas as pd
pd.DataFrame(columns=[
    'party_id','seed','schedule_hash','binary_hash','n_moves',
    'attribution_coverage','jaccard_div','cosine_div','rank_corr',
    'mass_shift','flags_triggered','L_half1','L_half2','delta_L','duration_s'
]).to_csv('$PARTIES_CSV', index=False)
pd.DataFrame(columns=[
    'party_id','tick','t_ms','port_from','port_to','cause_code',
    'attribution_p','top_driver','top_driver_weight','half'
]).to_csv('$MOVES_CSV', index=False)
pd.DataFrame(columns=[
    'party_id','half','driver','coefficient','selected','mi_score'
]).to_csv('$DRIVERS_CSV', index=False)
"

PARTY_ID=0
for seed in $(seq "$SEED_START" "$SEED_END"); do
    JOURNAL="$VALIDATE_JOURNALS_DIR/seed_${seed}.csv"
    if [[ ! -f "$JOURNAL" || ! -s "$JOURNAL" ]]; then
        echo "Warning: missing journal for seed $seed, skipping"
        continue
    fi

    PARTY_ID=$((PARTY_ID + 1))
    echo "Analyzing held-out party $PARTY_ID (seed $seed)..."

    python3 tools/atlas_analyze.py "$JOURNAL" "$PARTY_ID" "$seed" "$SCHEDULE_HASH" "$BINARY_HASH" \
        > "$OUT_DIR/tmp_analysis.txt" 2>&1

    python3 << 'PYEOF'
import sys, pandas as pd

with open('$OUT_DIR/tmp_analysis.txt', 'r') as f:
    content = f.read()

sections = content.split('===')
for i, name in enumerate(['PARTIES', 'MOVES', 'DRIVERS'], 1):
    if i < len(sections):
        lines = sections[i].strip().split('\n')
        if len(lines) > 1:
            header = lines[0]
            data = '\n'.join(lines[1:])
            df = pd.read_csv(pd.io.common.StringIO(data))
            df.to_csv(f'$OUT_DIR/atlas_{name.lower()}.csv', mode='a', header=False, index=False)
PYEOF

done

echo "=== ATLAS VALIDATE COMPLETE ==="
echo "Held-out parties analyzed: $PARTY_ID"
echo "Outputs:"
echo "  $PARTIES_CSV"
echo "  $MOVES_CSV"
echo "  $DRIVERS_CSV"

# Generate manifest
./tools/generate_atlas_manifest.sh "$OUT_DIR" "validate" "$SEED_START..$SEED_END" "$SCHEDULE_FILE" "$BINARY_HASH"

# Compare with training (report only, no threshold tuning)
if [[ -f "results/atlas_train/atlas_parties.csv" ]]; then
    echo ""
    echo "=== TRAIN vs VALIDATE COMPARISON ==="
    python3 << 'PYEOF'
import pandas as pd
import numpy as np

train = pd.read_csv('results/atlas_train/atlas_parties.csv')
valid = pd.read_csv('$OUT_DIR/atlas_parties.csv')

metrics = ['attribution_coverage', 'jaccard_div', 'cosine_div', 'rank_corr', 'mass_shift', 'L_half1', 'L_half2', 'delta_L']
for m in metrics:
    if m in train.columns and m in valid.columns:
        t_mean = train[m].mean()
        v_mean = valid[m].mean()
        rel_drop = (t_mean - v_mean) / t_mean * 100 if t_mean != 0 else float('inf')
        print(f"  {m}: train={t_mean:.4f} valid={v_mean:.4f} drop={rel_drop:.1f}%")
        if rel_drop > 15:
            print(f"    *** NEGATIVE RESULT: >15% drop on held-out ({rel_drop:.1f}%) ***")
PYEOF
fi

END_TIME=$(date -Is)
echo "End: $END_TIME"
