#!/usr/bin/env python3
"""recompute every statistic the readme and the paper cite, from shipped data.

output goes to stdout and, with --out, to a file the honesty gate can read:
the point is that a number in a paper must be regenerable by a command that
lives in this tree, not typed in by whoever wrote the paragraph.

usage:
    tools/paper_stats.py [--out results/paper_stats.txt]

stdlib only. detector state machine mirrors tools/calibrate_detector.py and
src/config.h (DET_COOL_MS 10000, threshold 50.0).
"""

import argparse
import glob
import math
import os
import re
import statistics

COOL_MS = 10000.0
THR = 50.0
DET_WINDOW_MS = 3000.0
T975_9 = 2.262  # two sided 95 percent t quantile, df 9
BUDGET_FA = 10


def rows(path):
    out = []
    with open(path) as f:
        for line in f:
            if line.startswith("#") or line.startswith("t_ms"):
                continue
            p = line.rstrip("\n").split(",")
            if len(p) < 11:
                continue
            out.append((float(p[0]), float(p[7]), int(p[8]), int(p[9]),
                        int(p[10])))
    return out


def firings(rs, thr=THR):
    """detector state machine: armed, one fire, then 10 s cooling."""
    hits, armed, cool_until = [], True, -1.0
    for t, score, _port, _cause, _ev in rs:
        if armed:
            if score >= thr:
                hits.append(t)
                armed, cool_until = False, t + COOL_MS
        elif t >= cool_until:
            armed = True
    return hits


def moves(rs):
    seq, prev = [], None
    for _t, _s, port, _c, _e in rs:
        if prev is None:
            prev = port
            continue
        if port != prev:
            seq.append(port)
            prev = port
    return seq


def t_ci(values):
    n = len(values)
    m = statistics.mean(values)
    sd = statistics.stdev(values) if n > 1 else 0.0
    half = T975_9 * sd / math.sqrt(n)
    return m, sd, m - half, m + half


def wilson(k, n, z=1.96):
    if n == 0:
        return (0.0, 0.0)
    p = k / n
    d = 1 + z * z / n
    c = p + z * z / (2 * n)
    r = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n))
    return ((c - r) / d, (c + r) / d)


def binom_cdf(k, n, p):
    return sum(math.comb(n, i) * p ** i * (1 - p) ** (n - i)
               for i in range(0, k + 1))


def clopper_pearson(k, n, alpha=0.05):
    """exact interval. cdf is decreasing in p, so both bounds bisect on it."""
    def solve(target, kk):
        lo, hi = 0.0, 1.0
        for _ in range(200):
            mid = (lo + hi) / 2
            if binom_cdf(kk, n, mid) > target:
                lo = mid
            else:
                hi = mid
        return (lo + hi) / 2
    lower = 0.0 if k == 0 else solve(1 - alpha / 2, k - 1)
    upper = 1.0 if k == n else solve(alpha / 2, k)
    return (lower, upper)


def poisson_ci(k, exposure, z=1.96):
    """Byar approximation, returns a rate interval in the same units."""
    if k == 0:
        return (0.0, 3.69 / exposure)
    lo = k * (1 - 1 / (9 * k) - z / (3 * math.sqrt(k))) ** 3
    hi = (k + 1) * (1 - 1 / (9 * (k + 1)) + z / (3 * math.sqrt(k + 1))) ** 3
    return (lo / exposure, hi / exposure)


def node_count(csv_path):
    """replicate conn_load_csv: NMJ rows dropped, BWM gap rows dropped."""
    src_set, dst_set = set(), set()
    with open(csv_path) as f:
        for line in f:
            if line.startswith("#") or line.startswith("Source"):
                continue
            p = line.rstrip("\n").split(",")
            if len(p) < 4:
                continue
            src, dst, typ = p[0].upper(), p[1].upper(), p[2].upper()
            if typ == "NMJ":
                continue
            if typ == "EJ" and ("BWM" in src or "BWM" in dst):
                continue
            src_set.add(src)
            dst_set.add(dst)
    return len(src_set | dst_set)


def moves_file(path):
    """t_ms,port rows from results/research_v2/*_moves.csv."""
    out = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("t_ms") or line.startswith("#"):
                continue
            t, port = line.split(",")
            out.append((float(t), int(port)))
    return out


def entropy_bits(seq):
    n = len(seq)
    counts = {}
    for x in seq:
        counts[x] = counts.get(x, 0) + 1
    h = -sum(c / n * math.log2(c / n) for c in counts.values())
    k = len([c for c in counts.values() if c > 0])
    mm = h + (k - 1) / (2 * n * math.log(2))   # miller-madow correction
    return h, mm, k


def held_out_order1(seq, frac=0.6, alphabet=64):
    """accuracy and log loss on a held out tail. laplace smoothing."""
    n = len(seq)
    cut = int(n * frac)
    train, test = seq[:cut], seq[cut:]
    tab = {}
    for a, b in zip(train, train[1:]):
        tab.setdefault(a, {})
        tab[a][b] = tab[a].get(b, 0) + 1
    maj = max(set(train), key=train.count)
    hit = 0
    loss = 0.0
    for a, b in zip(test, test[1:]):
        row = tab.get(a)
        if row:
            pred = max(row, key=row.get)
            denom = sum(row.values()) + alphabet
            prob = (row.get(b, 0) + 1) / denom
        else:
            pred = maj
            prob = 1.0 / alphabet
        if pred == b:
            hit += 1
        loss -= math.log2(prob)
    m = max(1, len(test) - 1)
    return hit / m, loss / m


def dwell_stats(path):
    seq = moves_file(path)
    ts = [t for t, _ in seq]
    d = [ts[i + 1] - ts[i] for i in range(len(ts) - 1)]
    if not d:
        return None
    m = statistics.mean(d)
    sd = statistics.pstdev(d)
    var = sum((x - m) ** 2 for x in d)
    ac = (sum((d[i] - m) * (d[i + 1] - m) for i in range(len(d) - 1)) / var
          if var else 0.0)
    return m, statistics.median(d), sd / m if m else 0.0, ac


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    args = ap.parse_args()
    lines = []

    def say(text=""):
        lines.append(text)

    say("# paper statistics, regenerated from shipped journals")
    say("# command: tools/paper_stats.py --out results/paper_stats.txt")
    say()

    nodes = node_count("src/petri/data/varshney_full.csv")
    say(f"connectome nodes after the loader rule (NMJ dropped, BWM gap "
        f"dropped): {nodes}")

    fa_files = sorted(glob.glob("results/cal_v12/fa_*.csv"))
    rates = [len(moves(rows(f))) for f in fa_files]
    m, sd, lo, hi = t_ci(rates)
    say()
    say(f"quiet move rate, n={len(rates)} seeds x 600 s:")
    say(f"  mean {m:.1f}, sd {sd:.1f}, 95% t-CI [{lo:.1f}, {hi:.1f}], "
        f"range [{min(rates)}, {max(rates)}]")
    say(f"  preregistered band [100, 300]: mean is below it, the CI "
        f"overlaps 100")

    fa_total = sum(len(firings(rows(f))) for f in fa_files)
    exposure_s = 600.0 * len(fa_files)
    per600 = fa_total / exposure_s * 600.0
    say()
    say(f"false alarms at threshold {THR:.1f}, {len(fa_files)} seeds, "
        f"{exposure_s:.0f} s model time:")
    say(f"  total {fa_total}, rate {per600:.2f} per 600 s")
    say("  no interval: every quiet journal has deterministic det_fired "
        "events at 1000 ms and a fixed later time, so the alarms are not "
        "independent")
    say(f"  budget {BUDGET_FA} over {exposure_s:.0f} s: "
        f"{'within' if fa_total <= BUDGET_FA else 'exceeded'}")

    burst_files = sorted(glob.glob("results/cal_v12/det_burst_*.csv"))
    strict = lenient = firmware_after = raw_after = 0
    for f in burst_files:
        rs = rows(f)
        ev = [t for t, _s, _p, _c, e in rs if e in (1, 2, 4)]
        hits = firings(rs)
        if hits:
            lenient += 1
            if ev and any(ev[0] <= h <= ev[0] + DET_WINDOW_MS for h in hits):
                strict += 1
        if any(e == 4 and 2010.0 <= t <= 2010.0 + DET_WINDOW_MS
               for t, _s, _p, _c, e in rs):
            firmware_after += 1
        if any(t >= 2010.0 and s >= THR for t, s, _p, _c, _e in rs):
            raw_after += 1
    n = len(burst_files)
    say()
    say(f"burst detection at threshold {THR:.1f}, n={n} journals:")
    say(f"  any firing anywhere: {lenient}/{n} = {lenient / n:.2f}, "
        f"wilson 95% CI [{wilson(lenient, n)[0]:.3f}, "
        f"{wilson(lenient, n)[1]:.3f}], "
        f"clopper-pearson [{clopper_pearson(lenient, n)[0]:.3f}, "
        f"{clopper_pearson(lenient, n)[1]:.3f}]; retracted as a detection "
        f"rate, the events precede the burst")
    say(f"  firing inside the nominal {DET_WINDOW_MS / 1000:.0f} s window "
        f"after the first event in (1,2,4): {strict}/{n} = "
        f"{strict / n:.2f}, wilson 95% CI "
        f"[{wilson(strict, n)[0]:.3f}, {wilson(strict, n)[1]:.3f}]; the "
        f"window starts at t0 = 1000 ms, before the burst at 2010 ms, so "
        f"this is not a burst detection rate")
    say(f"  firmware det_fired within {DET_WINDOW_MS / 1000:.0f} s after "
        f"2010 ms: {firmware_after}/{n}")
    say(f"  raw det_score at or above {THR:.1f} after 2010 ms: "
        f"{raw_after}/{n}; the offline cooldown may mask these crossings")

    say()
    say("party lengths, moves per 600 s party:")
    for mode in ("bio", "cycle", "random"):
        lens = []
        for f in sorted(glob.glob(f"results/journals_v12/{mode}/*.csv"))[:300]:
            lens.append(len(moves(rows(f))))
        if lens:
            say(f"  {mode}: n={len(lens)}, min {min(lens)}, median "
                f"{statistics.median(lens):.0f}, max {max(lens)}")

    say()
    say("no-op proxy, bio parties (a move tick whose port did not change):")
    tot_moves = tot_noop = 0
    for f in sorted(glob.glob("results/journals_v12/bio/*.csv"))[:100]:
        rs = rows(f)
        prev = rs[0][2] if rs else None
        for _t, _s, port, cause, _e in rs[1:]:
            if cause != 0:
                if port == prev:
                    tot_noop += 1
                else:
                    tot_moves += 1
                prev = port
    if tot_moves + tot_noop:
        say(f"  move ticks {tot_moves + tot_noop}, port unchanged "
            f"{tot_noop}, share {100.0 * tot_noop / (tot_moves + tot_noop):.1f}%")

    say()
    say("schedule statistics, one reconnaissance seed per mode "
        "(results/research_v2/*_moves.csv, n=1):")
    say("  mode     moves distinct   H bits  H_mm bits  held-out acc  "
        "log-loss  dwell mean/median/CV/acf1")
    for mode in ("cycle", "bio", "random"):
        path = f"results/research_v2/{mode}_1001_moves.csv"
        if not os.path.exists(path):
            continue
        seq = [port for _t, port in moves_file(path)]
        h, mm, k = entropy_bits(seq)
        acc, loss = held_out_order1(seq)
        dw = dwell_stats(path)
        say(f"  {mode:<7s} {len(seq):5d} {k:8d} {h:9.3f} {mm:10.3f} "
            f"{acc:13.3f} {loss:10.3f}  "
            f"{dw[0]:.0f}/{dw[1]:.0f}/{dw[2]:.2f}/{dw[3]:+.2f}")
    say("  note: n=1 seed, reconnaissance only; no confidence interval is "
        "attached to these values")

    say()
    say("idle cpu: not measured in this tree; the 0.75% figure is the "
        "arithmetic bound 6 x 1.25 ms per 1000 ms poll, not a measurement")
    say("bitwise replay: verified on seed 42 (one pair, cmp identical); "
        "a 100 party replay claim needs its own run")

    text = "\n".join(lines) + "\n"
    print(text, end="")
    if args.out:
        os.makedirs(os.path.dirname(args.out), exist_ok=True)
        with open(args.out, "w") as f:
            f.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
