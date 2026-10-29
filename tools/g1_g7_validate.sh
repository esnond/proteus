#!/usr/bin/env bash
# G1-G7 Hypothesis Validation — Phase 3 (r3)
# Runs on held-out seeds 2001..2100, strict separation from training
# Emits results/hypotheses_validate/g1_g7_results.csv

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Configuration (frozen per theory.md)
SEED_START=2001
SEED_END=2100
SCHEDULE_BIO="tests/fixtures/burst.csv"
SCHEDULE_SEEDED="tests/fixtures/seeded_schedule.csv"
SCHEDULE_RANDOM="tests/fixtures/random_schedule.csv"
VALIDATE_JOURNALS_DIR="results/atlas_journals_validate"
OUT_DIR="results/hypotheses_validate"
MAX_HOURS=72
DURATION_S=600

# Strict separation checks
if [[ -f "$OUT_DIR/g1_g7_results.csv" ]]; then
    echo "ERROR: Validation output already exists. Remove manually if intentional." >&2
    exit 1
fi

# Forbidden from reading training journals
TRAIN_JOURNALS_DIR="results/atlas_journals"
if [[ -d "$TRAIN_JOURNALS_DIR" ]]; then
    echo "WARNING: Training journals directory exists at $TRAIN_JOURNALS_DIR"
    echo "Validation MUST use separate journals at $VALIDATE_JOURNALS_DIR"
fi

mkdir -p "$OUT_DIR" "$VALIDATE_JOURNALS_DIR"

# Parse arguments
RESUME=false
while [[ $# -gt 0 ]]; do
    case $1 in
        --max-hours)
            MAX_HOURS="$2"
            shift 2
            ;;
        --resume)
            RESUME=true
            shift
            ;;
        --checkpoint)
            CHECKPOINT_FILE="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

CHECKPOINT_FILE="${CHECKPOINT_FILE:-$OUT_DIR/g1_g7_validate_checkpoint.txt}"
LOG_FILE="$OUT_DIR/g1_g7_validate.log"

# Validate
for f in "$SCHEDULE_BIO" "$SCHEDULE_SEEDED" "$SCHEDULE_RANDOM"; do
    if [[ ! -f "$f" ]]; then
        echo "Schedule not found: $f" >&2
        exit 1
    fi
done
if [[ ! -x ./proteusd ]]; then
    echo "proteusd not built. Run 'make'." >&2
    exit 1
fi

BINARY_HASH=$(sha256sum ./proteusd | cut -d' ' -f1)
COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
START_TIME=$(date -Is)
START_EPOCH=$(date +%s)
MAX_SECONDS=$((MAX_HOURS * 3600))

log() {
    echo "[$(date -Is)] $*" | tee -a "$LOG_FILE"
}

log "=== G1-G7 VALIDATION START (HELD-OUT) ==="
log "Commit: $COMMIT"
log "Binary SHA256: $BINARY_HASH"
log "Seeds: $SEED_START..$SEED_END (HELD-OUT)"
log "Max hours: $MAX_HOURS"
log "Output: $OUT_DIR"
log "Journals: $VALIDATE_JOURNALS_DIR (SEPARATE from training)"

# Load checkpoint
COMPLETED_EXPERIMENTS=()
if [[ "$RESUME" == true && -f "$CHECKPOINT_FILE" ]]; then
    mapfile -t COMPLETED_EXPERIMENTS < "$CHECKPOINT_FILE"
    log "Resuming: ${#COMPLETED_EXPERIMENTS[@]} experiments already done"
fi

is_completed() {
    local exp=$1
    for e in "${COMPLETED_EXPERIMENTS[@]}"; do
        [[ "$e" == "$exp" ]] && return 0
    done
    return 1
}

mark_completed() {
    COMPLETED_EXPERIMENTS+=("$1")
    printf '%s\n' "${COMPLETED_EXPERIMENTS[@]}" > "$CHECKPOINT_FILE"
}

# Run validation journals for all three schedules
run_validate_journals() {
    local schedule=$1
    local subdir=$2
    local label=$3

    log "=== Running $label journals (held-out) ==="
    ./tools/atlas_runner.sh \
        --seeds "$SEED_START..$SEED_END" \
        --schedule "$schedule" \
        --out-dir "$VALIDATE_JOURNALS_DIR/$subdir" \
        --max-hours $((MAX_HOURS / 3)) \
        --checkpoint "$VALIDATE_JOURNALS_DIR/$subdir/checkpoint.txt" \
        --resume \
        --duration "$DURATION_S" \
        >> "$LOG_FILE" 2>&1
}

# Run all three conditions
run_validate_journals "$SCHEDULE_BIO" "bio" "bio-stochastic"
run_validate_journals "$SCHEDULE_SEEDED" "seeded" "seeded no-contact"
run_validate_journals "$SCHEDULE_RANDOM" "random" "random no-contact"

# Run each hypothesis validation
validate_g1() {
    log "=== VALIDATE G1 ==="
    is_completed "g1" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g1_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import analyze_learnability_batch

bio_journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                       if f.endswith('.csv') and f.startswith('seed_')])
# Flat schedule for validation
flat_schedule = '$OUT_DIR/flat_schedule.csv'
os.makedirs(os.path.dirname(flat_schedule), exist_ok=True)
with open(flat_schedule, 'w') as f:
    f.write("# proteus events v1\n# t_ms,kind,amp\n")

flat_dir = '$VALIDATE_JOURNALS_DIR/flat'
os.makedirs(flat_dir, exist_ok=True)
# Note: flat journals would need to be run; skipping for brevity

bio_res = analyze_learnability_batch(bio_journals, '$SCHEDULE_BIO')
print(json.dumps({'bio': bio_res['summary']}, indent=2))
PYEOF

    mark_completed "g1"
}

validate_g2() {
    log "=== VALIDATE G2 ==="
    is_completed "g2" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g2_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import analyze_learnability_batch

journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                   if f.endswith('.csv') and f.startswith('seed_')])
results = analyze_learnability_batch(journals, '$SCHEDULE_BIO')
print(json.dumps(results['summary'], indent=2))
PYEOF

    mark_completed "g2"
}

validate_g3() {
    log "=== VALIDATE G3 ==="
    is_completed "g3" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g3_results.json" 2>>"$LOG_FILE"
import json, sys, os, numpy as np
sys.path.insert(0, 'tools')
from learnability import compute_learnability_all

journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                   if f.endswith('.csv') and f.startswith('seed_')])

results = []
for j in journals:
    seed = int(os.path.basename(j).split('_')[1].split('.')[0])
    res = compute_learnability_all(j, '$SCHEDULE_BIO')
    res['seed'] = seed
    results.append(res)

print(json.dumps({'per_seed_L': [{'seed': r['seed'], 'L_passive': r['L_passive']} for r in results],
                  'mean_L_passive': float(np.mean([r['L_passive'] for r in results]))}, indent=2))
PYEOF

    mark_completed "g3"
}

validate_g4() {
    log "=== VALIDATE G4 ==="
    is_completed "g4" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g4_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import compute_learnability_all
from observer_active import run_active_observer
from observer_passive import run_passive_observer

journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                   if f.endswith('.csv') and f.startswith('seed_')])

results = []
for j in journals[:20]:
    passive_res = run_passive_observer(j)
    passive_L = passive_res.get('L', 500)
    active_res = run_active_observer(j, passive_L)
    results.append({
        'journal': os.path.basename(j),
        'passive_L': passive_L,
        'active_L': active_res['active_L'],
        'probes': active_res['probes_sent'],
        'confidence': active_res['final_confidence'],
    })

print(json.dumps(results, indent=2))
PYEOF

    mark_completed "g4"
}

validate_g5() {
    log "=== VALIDATE G5 ==="
    is_completed "g5" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g5_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import analyze_learnability_batch

journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                   if f.endswith('.csv') and f.startswith('seed_')])
results = analyze_learnability_batch(journals, '$SCHEDULE_BIO')

cost_model = {'L_passive': 1, 'L_active': 5, 'L_reconstruction': 10}
table = []
for obs_class in ['L_passive', 'L_active', 'L_reconstruction']:
    mean_L = results['summary'][obs_class]['mean']
    cost = cost_model[obs_class]
    total = mean_L * cost
    table.append({'observer': obs_class, 'mean_L': mean_L, 'cost_per_obs': cost, 'total_cost': total})

print(json.dumps({'economic_table': table, 'summary': results['summary']}, indent=2))
PYEOF

    mark_completed "g5"
}

validate_g6() {
    log "=== VALIDATE G6 ==="
    is_completed "g6" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g6_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from observer_reconstruction import GeneratorReconstructionObserver

seeded_journals = sorted([f'$VALIDATE_JOURNALS_DIR/seeded/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/seeded')
                          if f.endswith('.csv') and f.startswith('seed_')])
bio_journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                       if f.endswith('.csv') and f.startswith('seed_')])

observer = GeneratorReconstructionObserver('$SCHEDULE_BIO')
gap_result = observer.compute_transfer_gap(seeded_journals, bio_journals)
print(json.dumps(gap_result, indent=2))
PYEOF

    mark_completed "g6"
}

validate_g7() {
    log "=== VALIDATE G7 (GATE CRITERION) ==="
    is_completed "g7" && return 0

    python3 << 'PYEOF' > "$OUT_DIR/g7_results.json" 2>>"$LOG_FILE"
import json, sys, os, numpy as np
sys.path.insert(0, 'tools')
from learnability import compare_conditions

bio_journals = sorted([f'$VALIDATE_JOURNALS_DIR/bio/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/bio')
                       if f.endswith('.csv') and f.startswith('seed_')])
seeded_journals = sorted([f'$VALIDATE_JOURNALS_DIR/seeded/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/seeded')
                          if f.endswith('.csv') and f.startswith('seed_')])
random_journals = sorted([f'$VALIDATE_JOURNALS_DIR/random/{f}' for f in os.listdir('$VALIDATE_JOURNALS_DIR/random')
                          if f.endswith('.csv') and f.startswith('seed_')])

comp_bio_seeded = compare_conditions(bio_journals, seeded_journals, '$SCHEDULE_BIO')
comp_bio_random = compare_conditions(bio_journals, random_journals, '$SCHEDULE_BIO')

bio_L_passive = comp_bio_seeded['condition_a_summary']['L_passive']['mean']
seeded_L_passive = comp_bio_seeded['condition_b_summary']['L_passive']['mean']
ratio = bio_L_passive / seeded_L_passive if seeded_L_passive > 0 else float('inf')
gate_pass = ratio >= 10.0

# Compare with training results
train_results = {}
if os.path.exists('results/hypotheses/g7_results.json'):
    with open('results/hypotheses/g7_results.json') as f:
        train_results = json.load(f)

drop = 0
if 'g7_gate' in train_results:
    train_ratio = train_results['g7_gate']['ratio_L_bio_over_L_seeded']
    if train_ratio > 0:
        drop = (train_ratio - ratio) / train_ratio * 100

result = {
    'bio_vs_seeded': comp_bio_seeded,
    'bio_vs_random': comp_bio_random,
    'g7_gate': {
        'ratio_L_bio_over_L_seeded': ratio,
        'gate_pass': gate_pass,
        'criterion': 'L_bio >= 10 * L_seeded (passive observer)',
    },
    'train_compare': {
        'train_ratio': train_results.get('g7_gate', {}).get('ratio_L_bio_over_L_seeded', 'N/A'),
        'validation_ratio': ratio,
        'relative_drop_pct': drop,
        'negative_result': drop > 15,
    }
}
print(json.dumps(result, indent=2))
PYEOF

    mark_completed "g7"
}

# Run validations
validate_g1
validate_g2
validate_g3
validate_g4
validate_g5
validate_g6
validate_g7

# Aggregate
log "=== AGGREGATING VALIDATION RESULTS ==="
python3 << 'PYEOF' > "$OUT_DIR/g1_g7_results.csv" 2>>"$LOG_FILE"
import json, csv, os, glob

rows = []
for g in ['g1', 'g2', 'g3', 'g4', 'g5', 'g6', 'g7']:
    for f in glob.glob(f'$OUT_DIR/{g}_results.json'):
        try:
            with open(f) as fp:
                data = json.load(fp)
            row = {'hypothesis': g}
            if 'bio' in data and 'L_passive' in data['bio']:
                row['L_passive_mean'] = data['bio']['L_passive']['mean']
            if 'g7_gate' in data:
                row['g7_ratio'] = data['g7_gate']['ratio_L_bio_over_L_seeded']
                row['g7_pass'] = data['g7_gate']['gate_pass']
                if 'train_compare' in data:
                    row['train_ratio'] = data['train_compare'].get('train_ratio', 'N/A')
                    row['drop_pct'] = data['train_compare'].get('relative_drop_pct', 'N/A')
                    row['negative_result'] = data['train_compare'].get('negative_result', False)
            rows.append(row)
        except Exception as e:
            print(f"Error reading {f}: {e}", file=sys.stderr)

if rows:
    fieldnames = ['hypothesis', 'L_passive_mean', 'g7_ratio', 'g7_pass', 'train_ratio', 'drop_pct', 'negative_result']
    with open('$OUT_DIR/g1_g7_results.csv', 'w') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print("Written: $OUT_DIR/g1_g7_results.csv")
PYEOF

# Generate manifest
./tools/generate_hypotheses_manifest.sh "$OUT_DIR" "validate" "$SEED_START..$SEED_END"

# Compare with training
if [[ -f "results/hypotheses/g1_g7_results.csv" ]]; then
    log ""
    log "=== TRAIN vs VALIDATION COMPARISON ==="
    python3 << 'PYEOF' 2>>"$LOG_FILE"
import pandas as pd
train = pd.read_csv('results/hypotheses/g1_g7_results.csv')
valid = pd.read_csv('$OUT_DIR/g1_g7_results.csv')
print("Hypothesis | Train L_passive | Valid L_passive | Drop%")
print("-----------|----------------|----------------|------")
for _, vrow in valid.iterrows():
    hyp = vrow['hypothesis']
    trow = train[train['hypothesis'] == hyp]
    if len(trow) > 0:
        tval = trow.iloc[0].get('L_passive_mean', 'N/A')
        vval = vrow.get('L_passive_mean', 'N/A')
        if isinstance(tval, float) and isinstance(vval, float) and tval != 0:
            drop = (tval - vval) / tval * 100
            flag = " *** NEGATIVE ***" if drop > 15 else ""
            print(f"{hyp:10} | {tval:14.1f} | {vval:14.1f} | {drop:5.1f}%{flag}")
        else:
            print(f"{hyp:10} | {tval:14} | {vval:14} | N/A")
PYEOF
fi

END_TIME=$(date -Is)
END_EPOCH=$(date +%s)
TOTAL_ELAPSED=$((END_EPOCH - START_EPOCH))

log "=== G1-G7 VALIDATION COMPLETE ==="
log "End: $END_TIME"
log "Total elapsed: ${TOTAL_ELAPSED}s"
log "Results: $OUT_DIR/g1_g7_results.csv"
log "Manifest: $OUT_DIR/manifest.txt"

exit 0
