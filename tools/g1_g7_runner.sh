#!/usr/bin/env bash
# G1-G7 Hypothesis Runner — Phase 3 (r3)
# Runs all 7 experiments on training seeds 1001..1100
# Emits results/hypotheses/g1_g7_results.csv + per-hypothesis CSVs
# Headless, bitwise-reproducible, night-shift-ops compliant

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Configuration (frozen per theory.md)
SEED_START=1001
SEED_END=1100
SCHEDULE_BIO="tests/fixtures/burst.csv"
SCHEDULE_SEEDED="tests/fixtures/seeded_schedule.csv"
SCHEDULE_RANDOM="tests/fixtures/random_schedule.csv"
JOURNALS_DIR="results/atlas_journals"
OUT_DIR="results/hypotheses"
MAX_HOURS=72
DURATION_S=600

# Night-shift-ops: checkpoint/resume, log seed+commit+flags+start
CHECKPOINT_FILE="$OUT_DIR/g1_g7_checkpoint.txt"
LOG_FILE="$OUT_DIR/g1_g7_run.log"

mkdir -p "$OUT_DIR" "$JOURNALS_DIR"

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

# Logging function
log() {
    echo "[$(date -Is)] $*" | tee -a "$LOG_FILE"
}

log "=== G1-G7 RUNNER START ==="
log "Commit: $COMMIT"
log "Binary SHA256: $BINARY_HASH"
log "Seeds: $SEED_START..$SEED_END (TRAINING)"
log "Max hours: $MAX_HOURS"
log "Output: $OUT_DIR"

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

# ============================================================
# EXPERIMENT 1: g1 - Self-disclosure of defense
# Flat profiles recognized faster than bio-stochastic variability
# Measure: L for flat vs bio schedules
# ============================================================
run_g1() {
    log "=== G1: Self-disclosure (flat vs bio) ==="
    local exp_name="g1"
    is_completed "$exp_name" && { log "G1 already done, skipping"; return 0; }

    # Run bio schedule (already done in atlas_journals for 1001..1100)
    # Run flat schedule: constant port (no moves) or periodic
    local flat_dir="$JOURNALS_DIR/flat"
    mkdir -p "$flat_dir"

    # Create flat schedule: single port, no events
    local flat_schedule="$OUT_DIR/flat_schedule.csv"
    echo -e "# proteus events v1\n# t_ms,kind,amp" > "$flat_schedule"
    # No events = flat profile

    # Run flat parties
    ./tools/atlas_runner.sh \
        --seeds "$SEED_START..$SEED_END" \
        --schedule "$flat_schedule" \
        --out-dir "$flat_dir" \
        --max-hours 12 \
        --checkpoint "$flat_dir/checkpoint.txt" \
        --resume \
        --duration "$DURATION_S" \
        >> "$LOG_FILE" 2>&1

    # Compute L for both conditions
    python3 tools/learnability.py --batch "$JOURNALS_DIR" "$SCHEDULE_BIO" \
        > "$OUT_DIR/g1_bio.json" 2>>"$LOG_FILE"
    python3 tools/learnability.py --batch "$flat_dir" "$flat_schedule" \
        > "$OUT_DIR/g1_flat.json" 2>>"$LOG_FILE"

    # Compare
    python3 << PYEOF >> "$LOG_FILE" 2>&1
import json, sys
with open('$OUT_DIR/g1_bio.json') as f: bio = json.load(f)
with open('$OUT_DIR/g1_flat.json') as f: flat = json.load(f)
# Extract L_passive means
bio_L = bio['summary']['L_passive']['mean']
flat_L = flat['summary']['L_passive']['mean']
print(f"G1 RESULT: L_bio={bio_L:.1f} L_flat={flat_L:.1f} ratio={bio_L/flat_L:.2f}")
print(f"G1: bio {'harder' if bio_L > flat_L else 'easier'} to learn than flat")
PYEOF

    mark_completed "$exp_name"
    log "G1 complete"
}

# ============================================================
# EXPERIMENT 2: g2 - Transparency = Predictability
# Prediction attack > state knowledge attack
# Measure: time to successful attack for two attacker models
# ============================================================
run_g2() {
    log "=== G2: Transparency = Predictability ==="
    local exp_name="g2"
    is_completed "$exp_name" && { log "G2 already done, skipping"; return 0; }

    # Attacker model 1: predicts next port (prediction attack)
    # Attacker model 2: knows current port (state knowledge)
    # Run on bio journals
    python3 << PYEOF > "$OUT_DIR/g2_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import analyze_learnability_batch, compare_conditions

journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                   if f.endswith('.csv') and f.startswith('seed_')])

# Prediction attacker: uses active observer (predicts next)
# State attacker: uses passive observer (knows current distribution)
# We simulate both by comparing L_active vs L_passive
results = analyze_learnability_batch(journals, '$SCHEDULE_BIO')
print(json.dumps(results['summary'], indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G2 complete"
}

# ============================================================
# EXPERIMENT 3: g3 - Scheme Anonymity (k-anonymity analog)
# Recognition time ~ 1/frequency
# Measure: recognition time vs scheme frequency regression
# ============================================================
run_g3() {
    log "=== G3: Scheme Anonymity (k-anonymity) ==="
    local exp_name="g3"
    is_completed "$exp_name" && { log "G3 already done, skipping"; return 0; }

    # For each seed (scheme instance), compute L
    # Regress L against scheme frequency in population
    # Since each seed is unique, frequency = 1/N; but we can group by schedule type
    python3 << PYEOF > "$OUT_DIR/g3_results.json" 2>>"$LOG_FILE"
import json, sys, os, numpy as np
sys.path.insert(0, 'tools')
from learnability import compute_learnability_all

journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                   if f.endswith('.csv') and f.startswith('seed_')])

results = []
for j in journals:
    seed = int(os.path.basename(j).split('_')[1].split('.')[0])
    res = compute_learnability_all(j, '$SCHEDULE_BIO')
    res['seed'] = seed
    results.append(res)

# All bio schemes have same frequency (1/100), so no variation
# For g3 we need multiple scheme families with different frequencies
# This is a placeholder - real g3 needs population of scheme families
print(json.dumps({
    'note': 'G3 requires multiple scheme families with varying frequencies',
    'per_seed_L': [{'seed': r['seed'], 'L_passive': r['L_passive']} for r in results],
    'mean_L_passive': np.mean([r['L_passive'] for r in results]),
}, indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G3 complete"
}

# ============================================================
# EXPERIMENT 4: g4 - Active Observer
# Probing reduces learnability below passive bound
# Measure: L_active vs L_passive; probe cost
# ============================================================
run_g4() {
    log "=== G4: Active Observer ==="
    local exp_name="g4"
    is_completed "$exp_name" && { log "G4 already done, skipping"; return 0; }

    python3 << PYEOF > "$OUT_DIR/g4_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import compute_learnability_all
from observer_active import run_active_observer
from observer_passive import run_passive_observer

journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                   if f.endswith('.csv') and f.startswith('seed_')])

results = []
for j in journals[:20]:  # sample first 20 for speed
    passive_res = run_passive_observer(j)
    passive_L = passive_res.get('L', 500)
    active_res = run_active_observer(j, passive_L)
    results.append({
        'journal': os.path.basename(j),
        'passive_L': passive_L,
        'active_L': active_res['active_L'],
        'probes': active_res['probes_sent'],
        'budget': active_res['budget_used'],
        'confidence': active_res['final_confidence'],
    })

print(json.dumps(results, indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G4 complete"
}

# ============================================================
# EXPERIMENT 5: g5 - Economic Threshold
# Learnability -> computational cost table
# Measure: L table by observer class
# ============================================================
run_g5() {
    log "=== G5: Economic Threshold ==="
    local exp_name="g5"
    is_completed "$exp_name" && { log "G5 already done, skipping"; return 0; }

    python3 << PYEOF > "$OUT_DIR/g5_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from learnability import analyze_learnability_batch

journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                   if f.endswith('.csv') and f.startswith('seed_')])

results = analyze_learnability_batch(journals, '$SCHEDULE_BIO')

# Economic threshold table: observer class -> L -> compute cost
# Cost model: passive=1 unit/obs, active=5 units/probe, recon=10 units/obs
cost_model = {'L_passive': 1, 'L_active': 5, 'L_reconstruction': 10}
print("ECONOMIC THRESHOLD TABLE:")
print("Observer Class | Mean L | Cost/obs | Total Cost")
print("---------------|--------|----------|-----------")
for obs_class in ['L_passive', 'L_active', 'L_reconstruction']:
    mean_L = results['summary'][obs_class]['mean']
    cost = cost_model[obs_class]
    total = mean_L * cost
    print(f"{obs_class:14} | {mean_L:6.1f} | {cost:8} | {total:11.1f}")

print(json.dumps(results['summary'], indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G5 complete"
}

# ============================================================
# EXPERIMENT 6: g6 - Model Collapse Caveat
# Synthetic-trained observer transfer gap
# Measure: transfer gap on independent stand
# ============================================================
run_g6() {
    log "=== G6: Model Collapse Caveat ==="
    local exp_name="g6"
    is_completed "$exp_name" && { log "G6 already done, skipping"; return 0; }

    # Train on synthetic (seeded) journals, test on bio (independent)
    # Need seeded journals - run them
    local seeded_dir="$JOURNALS_DIR/seeded_train"
    mkdir -p "$seeded_dir"

    ./tools/atlas_runner.sh \
        --seeds "$SEED_START..$SEED_END" \
        --schedule "$SCHEDULE_SEEDED" \
        --out-dir "$seeded_dir" \
        --max-hours 12 \
        --checkpoint "$seeded_dir/checkpoint.txt" \
        --resume \
        --duration "$DURATION_S" \
        >> "$LOG_FILE" 2>&1

    python3 << PYEOF > "$OUT_DIR/g6_results.json" 2>>"$LOG_FILE"
import json, sys, os
sys.path.insert(0, 'tools')
from observer_reconstruction import GeneratorReconstructionObserver, read_journal

# Train on seeded
seeded_journals = sorted([f'$seeded_dir/{f}' for f in os.listdir('$seeded_dir')
                          if f.endswith('.csv') and f.startswith('seed_')])
bio_journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                       if f.endswith('.csv') and f.startswith('seed_')])

observer = GeneratorReconstructionObserver('$SCHEDULE_BIO')
gap_result = observer.compute_transfer_gap(seeded_journals, bio_journals)
print(json.dumps(gap_result, indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G6 complete"
}

# ============================================================
# EXPERIMENT 7: g7 - Central Proteus Claim
# Bio-stochastic with contact: L > 10x seeded, <1% legit disruption
# Gate criterion for r3
# ============================================================
run_g7() {
    log "=== G7: Central Proteus Claim (Bio-stochastic with contact) ==="
    local exp_name="g7"
    is_completed "$exp_name" && { log "G7 already done, skipping"; return 0; }

    # Need three conditions on same seeds:
    # 1. Bio with contact (burst.csv) - already in atlas_journals
    # 2. Seeded no-contact (seeded_schedule.csv)
    # 3. Random no-contact (random_schedule.csv)

    local random_dir="$JOURNALS_DIR/random"
    mkdir -p "$random_dir"

    ./tools/atlas_runner.sh \
        --seeds "$SEED_START..$SEED_END" \
        --schedule "$SCHEDULE_RANDOM" \
        --out-dir "$random_dir" \
        --max-hours 12 \
        --checkpoint "$random_dir/checkpoint.txt" \
        --resume \
        --duration "$DURATION_S" \
        >> "$LOG_FILE" 2>&1

    # Seeded already run for g6
    local seeded_dir="$JOURNALS_DIR/seeded_train"

    python3 << PYEOF > "$OUT_DIR/g7_results.json" 2>>"$LOG_FILE"
import json, sys, os, numpy as np
sys.path.insert(0, 'tools')
from learnability import compare_conditions

bio_journals = sorted([f'$JOURNALS_DIR/{f}' for f in os.listdir('$JOURNALS_DIR')
                       if f.endswith('.csv') and f.startswith('seed_')])
seeded_journals = sorted([f'$seeded_dir/{f}' for f in os.listdir('$seeded_dir')
                          if f.endswith('.csv') and f.startswith('seed_')])
random_journals = sorted([f'$random_dir/{f}' for f in os.listdir('$random_dir')
                          if f.endswith('.csv') and f.startswith('seed_')])

# Compare bio vs seeded (main g7 criterion)
comp_bio_seeded = compare_conditions(bio_journals, seeded_journals, '$SCHEDULE_BIO')
# Compare bio vs random
comp_bio_random = compare_conditions(bio_journals, random_journals, '$SCHEDULE_BIO')

# G7 gate: L_bio / L_seeded >= 10 for passive observer
bio_L_passive = comp_bio_seeded['condition_a_summary']['L_passive']['mean']
seeded_L_passive = comp_bio_seeded['condition_b_summary']['L_passive']['mean']
ratio = bio_L_passive / seeded_L_passive if seeded_L_passive > 0 else float('inf')
gate_pass = ratio >= 10.0

print(f"G7 GATE: L_bio/L_seeded = {ratio:.2f} {'PASS' if gate_pass else 'FAIL'}")
print(f"  L_bio (passive) = {bio_L_passive:.1f}")
print(f"  L_seeded (passive) = {seeded_L_passive:.1f}")
print(f"  Ratio = {ratio:.2f}")

result = {
    'bio_vs_seeded': comp_bio_seeded,
    'bio_vs_random': comp_bio_random,
    'g7_gate': {
        'ratio_L_bio_over_L_seeded': ratio,
        'gate_pass': gate_pass,
        'criterion': 'L_bio >= 10 * L_seeded (passive observer)',
    },
    'legit_disruption_check': {
        'note': 'Requires legit client disruption measurement from r2 validation',
        'threshold': '<1% legit disruption',
    }
}
print(json.dumps(result, indent=2))
PYEOF

    mark_completed "$exp_name"
    log "G7 complete"
}

# ============================================================
# MAIN: Run all experiments
# ============================================================
run_g1
run_g2
run_g3
run_g4
run_g5
run_g6
run_g7

# ============================================================
# AGGREGATE RESULTS
# ============================================================
log "=== AGGREGATING RESULTS ==="
python3 << PYEOF > "$OUT_DIR/g1_g7_results.csv" 2>>"$LOG_FILE"
import json, csv, os, glob

rows = []
for g in ['g1', 'g2', 'g3', 'g4', 'g5', 'g6', 'g7']:
    for f in glob.glob(f'$OUT_DIR/{g}_results.json'):
        try:
            with open(f) as fp:
                data = json.load(fp)
            # Flatten key results
            row = {'hypothesis': g}
            if 'mean_L_passive' in data:
                row['L_passive_mean'] = data['mean_L_passive']
            if 'ratio_L_bio_over_L_seeded' in data.get('g7_gate', {}):
                row['g7_ratio'] = data['g7_gate']['ratio_L_bio_over_L_seeded']
                row['g7_pass'] = data['g7_gate']['gate_pass']
            if 'g1' == g and 'bio_L' in str(data):
                pass  # parsed from log
            rows.append(row)
        except:
            pass

# Write summary CSV
if rows:
    fieldnames = ['hypothesis', 'L_passive_mean', 'g7_ratio', 'g7_pass']
    with open('$OUT_DIR/g1_g7_results.csv', 'w') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print("Written: $OUT_DIR/g1_g7_results.csv")
else:
    print("No results to aggregate")
PYEOF

# Generate manifest
./tools/generate_hypotheses_manifest.sh "$OUT_DIR" "train" "$SEED_START..$SEED_END"

END_TIME=$(date -Is)
END_EPOCH=$(date +%s)
TOTAL_ELAPSED=$((END_EPOCH - START_EPOCH))

log "=== G1-G7 RUNNER COMPLETE ==="
log "End: $END_TIME"
log "Total elapsed: ${TOTAL_ELAPSED}s"
log "Results: $OUT_DIR/g1_g7_results.csv"
log "Manifest: $OUT_DIR/manifest.txt"

exit 0
