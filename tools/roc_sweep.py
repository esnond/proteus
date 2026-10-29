#!/usr/bin/env python3
"""roc_sweep: offline ROC of the detector score against labeled traffic.

input: a directory with
  legit_S.csv   journal of a run with only legitimate sessions
  attack_S.csv  journal of a run with a scan burst
  counts.csv    lines "legit,S,<connections>" for the denominators

the detector's live threshold (30.0) is frozen; this sweep only reports
what FP/FN would be at other thresholds, it does not re-run the daemon.
a "firing" is a rising edge of det_score >= T, which approximates the
detector's armed/fired state machine (the live state also has a cooldown
and a refractory), stated here so the number is not overread.

usage: roc_sweep.py DIR [budget_per_10k]
"""
import math
import os
import sys


def crossings(path, thr):
    n = 0
    above = False
    for ln in open(path):
        if ln.startswith("#"):
            continue
        f = ln.strip().split(",")
        if len(f) < 11:
            continue
        try:
            score = float(f[7])
        except ValueError:
            continue
        now = score >= thr
        if now and not above:
            n += 1
        above = now
    return n


def main():
    d = sys.argv[1]
    budget = float(sys.argv[2]) if len(sys.argv) > 2 else 5.0
    legit = sorted(f for f in os.listdir(d) if f.startswith("legit_"))
    attack = sorted(f for f in os.listdir(d) if f.startswith("attack_"))
    legit_n = 0
    if os.path.exists(os.path.join(d, "counts.csv")):
        for ln in open(os.path.join(d, "counts.csv")):
            f = ln.strip().split(",")
            if len(f) == 3 and f[0] == "legit":
                legit_n += int(f[2])
    if not legit or not attack or legit_n == 0:
        print("insufficient: need legit_*.csv, attack_*.csv, counts.csv")
        return 2
    rows = []
    for thr in range(5, 61):
        fp_events = sum(crossings(os.path.join(d, f), thr) for f in legit)
        fp10k = fp_events / legit_n * 10000.0
        det = sum(1 for f in attack
                  if crossings(os.path.join(d, f), thr) > 0)
        rows.append((thr, fp_events, fp10k, det, len(attack)))
    print("thr,fp_events,fp_per_10k,detected,attack_runs")
    for r in rows:
        print("%d,%d,%.2f,%d,%d" % r)
    # smallest threshold that meets the budget (higher threshold = fewer FP)
    pick = None
    for thr, _, fp10k, det, n in rows:
        if fp10k <= budget:
            pick = (thr, fp10k, det, n)
    if pick is None:
        print("boundary: no threshold in 5..60 reaches FP<=%.1f per 10k" % budget)
        return 0
    thr, fp10k, det, n = pick
    fn = n - det
    # operational FP: the live state machine's bans on legit-only runs.
    # the rising-edge count above is a conservative score proxy; the two
    # differ because the live detector has cooldown and refractory.
    live_bans = sum(sum(1 for ln in open(os.path.join(d, f))
                        if not ln.startswith("#") and ln.count(",") >= 10
                        and ln.strip().split(",")[10] == "8")
                    for f in legit)
    total = max(1, legit_n)
    live10k = live_bans / total * 10000.0
    up = 3.0 / total * 10000.0 if live_bans == 0 else \
        wilson(live_bans, total, 1.96)[1] * 10000.0
    print("live_fp,bans=%d,sessions=%d,per_10k=%.2f,upper95_per_10k=%.2f"
          % (live_bans, total, live10k, up))
    print("operating_point,thr=%d,fp_per_10k=%.2f,detected=%d/%d,fn=%d"
          % (thr, fp10k, det, n, fn))
    ci_lo = wilson(det, n)[0]
    ci_hi = wilson(det, n)[1]
    print("detection_rate=%.2f,ci95=[%.2f,%.2f],n=%d" % (det / n, ci_lo, ci_hi, n))
    print("live_threshold=30.0,frozen_not_refit")
    return 0


def wilson(k, n, z=1.96):
    if n == 0:
        return (0.0, 0.0)
    p = k / n
    den = 1 + z * z / n
    c = (p + z * z / (2 * n)) / den
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / den
    return (max(0.0, c - h), min(1.0, c + h))


if __name__ == "__main__":
    sys.exit(main())
