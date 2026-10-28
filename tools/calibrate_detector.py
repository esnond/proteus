#!/usr/bin/env python3
# detector threshold calibration, offline reconstruction of the detector
# state machine from the det_score journal column. publishes the false
# alarm curve for the withheld seeds and, if burst journals are given,
# the detection side. the tool measures, the owner freezes the constant.
import sys, glob

COOL_MS = 10000  # must mirror DET_COOL_MS in src/config.h
BUDGET_FA = 10   # one per 10 minutes over the 6000 s of withheld runs


def firings(rows, thr):
    """replay the detector state machine over (t_ms, score) pairs, return
    the times where an armed detector crossed the threshold"""
    hits, armed, cool_until = [], True, -1.0
    for t, score in rows:
        if armed:
            if score >= thr:
                hits.append(t)
                armed, cool_until = False, t + COOL_MS
        elif t >= cool_until:
            armed = True
    return hits


def load(path):
    rows = []
    for ln in open(path):
        if ln.startswith("#"):
            continue
        f = ln.split(",")
        rows.append((float(f[0]), float(f[7]), int(f[10])))
    return rows


def main():
    fa_files = sorted(glob.glob(sys.argv[1] if len(sys.argv) > 1
                                else "results/fa_*.csv"))
    burst_files = sorted(glob.glob(sys.argv[2] if len(sys.argv) > 2
                                   else "results/det_burst_*.csv"))
    total_s = 600.0 * len(fa_files)
    print(f"# detector calibration, {len(fa_files)} withheld seeds,"
          f" {total_s:.0f} s model time, budget {BUDGET_FA} false alarms")
    print("# threshold,fa_total,fa_rate_per_600s,det_pos,det_n")
    grid = [20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 60.0, 70.0, 80.0, 100.0]
    for thr in grid:
        fa_total = sum(len(firings([(t, s) for t, s, _ in load(f)], thr))
                       for f in fa_files)
        det_pos = 0
        for f in burst_files:
            rows = load(f)
            ev_rows = [t for t, _, ev in rows if ev in (1, 2, 4)]
            if not ev_rows:
                continue
            t0 = ev_rows[0]
            hits = [h for h in firings([(t, s) for t, s, _ in rows], thr)
                    if t0 <= h <= t0 + 3000]
            if hits:
                det_pos += 1
        rate = fa_total / total_s * 600.0
        print(f"{thr:.1f},{fa_total},{rate:.2f},{det_pos},{len(burst_files)}")


if __name__ == "__main__":
    main()
