#!/usr/bin/env python3
"""
h5 sustained_load_fatigue — phase B (r3)

Registered claim (docs/hypotheses.md, written before this tool): the
detector discriminates burst-shaped scan from sustained constant-rate
scan of equal total dose; falsification = sustained fires significantly
fewer than burst.

Headless only, 600 s sim per party, seeds 1001..1020 (train set),
paired by seed. Dose matching is fixed before any run: sustained
schedule has the same number of events as the burst fixture and each
carries amp = total_burst_amp / n_events, spread evenly over the party.

Output: results/sustained/sustained_results.json
        results/sustained_journals/{burst,sustained}_seed_<n>.csv
"""

import json
import os
import subprocess
import sys

import numpy as np
from scipy import stats

BURST_FIXTURE = "tests/fixtures/burst.csv"
DURATION_S = 600
SEED_START = 1001
N_SEEDS = 20
OUT_DIR = "results/sustained"
JOURNAL_DIR = "results/sustained_journals"
N_BOOTSTRAP = 1000


def read_burst_dose(path):
    total = 0.0
    n = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(",")
            if len(parts) != 3:
                continue
            total += float(parts[2])
            n += 1
    return n, total


def write_sustained_schedule(path, n_events, total_amp):
    step_ms = (DURATION_S * 1000) // n_events
    amp = total_amp / n_events
    with open(path, "w") as f:
        f.write("# proteus events v1\n# t_ms,kind,amp\n")
        f.write("# sustained: %d events, total amp %.4f matched to %s\n"
                % (n_events, total_amp, BURST_FIXTURE))
        for i in range(1, n_events + 1):
            f.write("%d,burst,%.4f\n" % (i * step_ms, amp))


def run_party(binary, seed, schedule, out_csv):
    cmd = [binary, "-s", str(seed), "-d", str(DURATION_S),
           "-e", schedule, "-o", out_csv]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)


def journal_outcomes(path):
    det_fired = 0
    score_sum = 0.0
    score_n = 0
    last300_start = DURATION_S * 1000 - 300000
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.rstrip("\n").split(",")
            if len(parts) != 11:
                continue
            t_ms = float(parts[0])
            event = int(parts[10])
            if event == 4:
                det_fired += 1
            if t_ms >= last300_start:
                score_sum += float(parts[7])
                score_n += 1
    return {"det_fired": det_fired,
            "mean_det_score_last300s": score_sum / max(1, score_n)}


def mean_ci(x):
    x = np.asarray(x, dtype=float)
    n = len(x)
    rng = np.random.default_rng(20261029)
    boots = np.empty(N_BOOTSTRAP)
    for b in range(N_BOOTSTRAP):
        boots[b] = x[rng.integers(0, n, n)].mean()
    return float(x.mean()), float(np.percentile(boots, 2.5)), \
        float(np.percentile(boots, 97.5))


def paired_stats(burst_v, sust_v):
    burst_v = np.asarray(burst_v, dtype=float)
    sust_v = np.asarray(sust_v, dtype=float)
    diff = sust_v - burst_v
    m, lo, hi = mean_ci(diff)
    sd = float(diff.std(ddof=1)) if len(diff) > 1 else 0.0
    dz = float(diff.mean() / sd) if sd > 0 else 0.0
    try:
        stat, p = stats.wilcoxon(sust_v, burst_v)
    except ValueError:
        stat, p = float("nan"), 1.0
    return {"mean_diff_sustained_minus_burst": m,
            "ci95": [lo, hi], "n": len(diff), "sd_diff": sd,
            "cohens_dz": dz, "wilcoxon_W": float(stat),
            "wilcoxon_p": float(p)}


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(JOURNAL_DIR, exist_ok=True)
    binary = "./proteusd"
    if not os.access(binary, os.X_OK):
        sys.exit("proteusd not built; run make")

    n_events, total_amp = read_burst_dose(BURST_FIXTURE)
    sustained_path = os.path.join(OUT_DIR, "sustained_schedule.csv")
    write_sustained_schedule(sustained_path, n_events, total_amp)

    seeds = list(range(SEED_START, SEED_START + N_SEEDS))
    per_seed = []
    for seed in seeds:
        row = {"seed": seed}
        for cond, sched in (("burst", BURST_FIXTURE),
                            ("sustained", sustained_path)):
            csv = os.path.join(JOURNAL_DIR, "%s_seed_%d.csv" % (cond, seed))
            run_party(binary, seed, sched, csv)
            row[cond] = journal_outcomes(csv)
        per_seed.append(row)
        print("seed %d: burst fires=%d sustained fires=%d"
              % (seed, row["burst"]["det_fired"],
                 row["sustained"]["det_fired"]))

    result = {
        "hypothesis": "h5 sustained_load_fatigue",
        "registration": "docs/hypotheses.md r3 phase B",
        "burst_fixture": BURST_FIXTURE,
        "dose_match": {"n_events": n_events, "total_amp": total_amp,
                       "per_event_amp": total_amp / n_events},
        "n_seeds": N_SEEDS,
        "seeds": "%d..%d" % (seeds[0], seeds[-1]),
        "duration_s": DURATION_S,
        "alpha": 0.0083,
        "outcomes": per_seed,
        "stats": {
            "det_fired": paired_stats(
                [r["burst"]["det_fired"] for r in per_seed],
                [r["sustained"]["det_fired"] for r in per_seed]),
            "mean_det_score_last300s": paired_stats(
                [r["burst"]["mean_det_score_last300s"] for r in per_seed],
                [r["sustained"]["mean_det_score_last300s"]
                 for r in per_seed]),
        },
    }

    fired = [r["sustained"]["det_fired"] for r in per_seed]
    burst_fired = [r["burst"]["det_fired"] for r in per_seed]
    st = result["stats"]["det_fired"]
    degraded = (st["wilcoxon_p"] < result["alpha"]
                and st["mean_diff_sustained_minus_burst"] < 0)
    result["verdict"] = ("FALSIFIED: sustained load suppresses detection"
                         if degraded else
                         "NOT FALSIFIED: detector not degraded under "
                         "sustained matched dose")
    result["verdict_detail"] = {
        "burst_fires_total": int(sum(burst_fired)),
        "sustained_fires_total": int(sum(fired)),
        "degradation_test": "wilcoxon p < %.4f AND mean diff < 0"
                            % result["alpha"],
    }

    out = os.path.join(OUT_DIR, "sustained_results.json")
    with open(out, "w") as f:
        json.dump(result, f, indent=2)
    print("verdict:", result["verdict"])
    print("wrote", out)


if __name__ == "__main__":
    main()
