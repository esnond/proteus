#!/usr/bin/env bash
# ATLAS OF PARTIES runner — Phase 2 (r3)
# Runs N parties headless, emits journals to results/atlas_journals/seed_<N>.csv
# Bitwise-reproducible: xorshift64*, -ffp-contract=off, no wall clock in sim path
# Usage: ./tools/atlas_runner.sh [--seeds START..END] [--schedule FILE] [--out-dir DIR] [--max-hours H] [--checkpoint FILE] [--resume]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Defaults
SEED_START=1001
SEED_END=1100
SCHEDULE_FILE="tests/fixtures/burst.csv"
OUT_DIR="results/atlas_journals"
MAX_HOURS=48
CHECKPOINT_FILE=""
RESUME=false
DURATION_S=600

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --seeds)
            SEED_RANGE="$2"
            SEED_START="${SEED_RANGE%%..*}"
            SEED_END="${SEED_RANGE##*..}"
            shift 2
            ;;
        --schedule)
            SCHEDULE_FILE="$2"
            shift 2
            ;;
        --out-dir)
            OUT_DIR="$2"
            shift 2
            ;;
        --max-hours)
            MAX_HOURS="$2"
            shift 2
            ;;
        --checkpoint)
            CHECKPOINT_FILE="$2"
            shift 2
            ;;
        --resume)
            RESUME=true
            shift
            ;;
        --duration)
            DURATION_S="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

mkdir -p "$OUT_DIR"

# Validate schedule file
if [[ ! -f "$SCHEDULE_FILE" ]]; then
    echo "Schedule file not found: $SCHEDULE_FILE" >&2
    exit 1
fi

# Validate binary
if [[ ! -x ./proteusd ]]; then
    echo "proteusd binary not found. Run 'make' first." >&2
    exit 1
fi

BINARY_HASH=$(sha256sum ./proteusd | cut -d' ' -f1)
SCHEDULE_HASH=$(sha256sum "$SCHEDULE_FILE" | cut -d' ' -f1)
COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
START_TIME=$(date -Is)
START_EPOCH=$(date +%s)
MAX_SECONDS=$((MAX_HOURS * 3600))

echo "=== ATLAS RUNNER START ==="
echo "Commit: $COMMIT"
echo "Binary SHA256: $BINARY_HASH"
echo "Schedule: $SCHEDULE_FILE ($SCHEDULE_HASH)"
echo "Seeds: $SEED_START..$SEED_END"
echo "Duration: ${DURATION_S}s per party"
echo "Output: $OUT_DIR"
echo "Max hours: $MAX_HOURS"
echo "Start: $START_TIME"

# Load checkpoint if resuming
COMPLETED_SEEDS=()
if [[ "$RESUME" == true && -f "$CHECKPOINT_FILE" ]]; then
    mapfile -t COMPLETED_SEEDS < "$CHECKPOINT_FILE"
    echo "Resuming from checkpoint: ${#COMPLETED_SEEDS[@]} seeds already done"
fi

# Check if seed is already completed
is_completed() {
    local seed=$1
    for s in "${COMPLETED_SEEDS[@]}"; do
        if [[ "$s" == "$seed" ]]; then
            return 0
        fi
    done
    return 1
}

# Write checkpoint
write_checkpoint() {
    if [[ -n "$CHECKPOINT_FILE" ]]; then
        printf '%s\n' "${COMPLETED_SEEDS[@]}" > "$CHECKPOINT_FILE"
    fi
}

# Run single party
run_party() {
    local seed=$1
    local out_file="$OUT_DIR/seed_${seed}.csv"
    local log_file="$OUT_DIR/seed_${seed}.log"

    # Skip if already done and output exists
    if [[ -f "$out_file" && -s "$out_file" ]]; then
        echo "Seed $seed: journal exists, skipping"
        COMPLETED_SEEDS+=("$seed")
        write_checkpoint
        return 0
    fi

    echo "Running seed $seed..."
    local party_start=$(date +%s)

    # Run headless, no -r flag (determinism gate)
    ./proteusd -s "$seed" -d "$DURATION_S" -e "$SCHEDULE_FILE" -o "$out_file" \
        2>"$log_file"
    local rc=$?

    local party_end=$(date +%s)
    local party_duration=$((party_end - party_start))

    if [[ $rc -eq 0 ]]; then
        # Verify journal has data (not just header)
        local rows=$(tail -n +3 "$out_file" 2>/dev/null | wc -l)
        if [[ $rows -gt 0 ]]; then
            echo "Seed $seed: OK ($rows ticks, ${party_duration}s wall)"
            COMPLETED_SEEDS+=("$seed")
            write_checkpoint
            return 0
        else
            echo "Seed $seed: FAILED - empty journal" >&2
            return 1
        fi
    else
        echo "Seed $seed: FAILED (exit code $rc)" >&2
        return 1
    fi
}

# Main loop
SEEDS_TOTAL=$((SEED_END - SEED_START + 1))
SEEDS_DONE=0
SEEDS_FAILED=0

for seed in $(seq "$SEED_START" "$SEED_END"); do
    # Check time budget
    NOW=$(date +%s)
    ELAPSED=$((NOW - START_EPOCH))
    if [[ $ELAPSED -ge $MAX_SECONDS ]]; then
        echo "Time budget exhausted (${MAX_HOURS}h). Stopping at seed $seed."
        break
    fi

    # Skip completed
    if is_completed "$seed"; then
        SEEDS_DONE=$((SEEDS_DONE + 1))
        continue
    fi

    if run_party "$seed"; then
        SEEDS_DONE=$((SEEDS_DONE + 1))
    else
        SEEDS_FAILED=$((SEEDS_FAILED + 1))
    fi
done

END_TIME=$(date -Is)
END_EPOCH=$(date +%s)
TOTAL_ELAPSED=$((END_EPOCH - START_EPOCH))

echo "=== ATLAS RUNNER COMPLETE ==="
echo "End: $END_TIME"
echo "Total elapsed: ${TOTAL_ELAPSED}s"
echo "Seeds processed: $SEEDS_DONE / $SEEDS_TOTAL"
echo "Seeds failed: $SEEDS_FAILED"
echo "Output directory: $OUT_DIR"

# Summary manifest
{
    echo "# Atlas runner summary"
    echo "# Generated: $END_TIME"
    echo "# Commit: $COMMIT"
    echo "# Binary SHA256: $BINARY_HASH"
    echo "# Schedule: $SCHEDULE_FILE ($SCHEDULE_HASH)"
    echo "# Seed range: $SEED_START..$SEED_END"
    echo "# Duration: $DURATION_S"
    echo "# Completed: $SEEDS_DONE"
    echo "# Failed: $SEEDS_FAILED"
    echo "# Elapsed seconds: $TOTAL_ELAPSED"
} > "$OUT_DIR/run_summary.txt"

if [[ $SEEDS_FAILED -gt 0 ]]; then
    exit 1
fi
exit 0
