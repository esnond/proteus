#!/usr/bin/env python3
"""analyse the n=30 per mode sweep: entropy, held-out prediction, dwell.

reads results/entropy_n30/summary.csv, analyses only parties the sweep marked
rc=0, and reports per mode mean with a bootstrap 95 percent interval over
seeds, plus the paired difference between bio and each control.

usage:
    tools/entropy_analysis.py [--dir results/entropy_n30] [--out FILE]
"""

import argparse
import csv
import glob
import math
import os
import random
import statistics

ALPHABET = 64
BOOT = 10000
SEED = 12345


def moves(path):
    out, prev = [], None
    with open(path) as f:
        for line in f:
            if line.startswith("#") or line.startswith("t_ms"):
                continue
            p = line.rstrip("\n").split(",")
            if len(p) < 10:
                continue
            t, port = float(p[0]), int(p[8])
            if prev is None:
                prev = port
                continue
            if port != prev:
                out.append((t, port))
                prev = port
    return out


def entropy(seq):
    n = len(seq)
    if n == 0:
        return 0.0, 0.0, 0
    counts = {}
    for x in seq:
        counts[x] = counts.get(x, 0) + 1
    h = -sum(c / n * math.log2(c / n) for c in counts.values())
    k = len(counts)
    return h, h + (k - 1) / (2 * n * math.log(2)), k


def held_out(seq, frac=0.6):
    n = len(seq)
    cut = int(n * frac)
    train, test = seq[:cut], seq[cut:]
    if len(test) < 2:
        return 0.0, 0.0
    tab = {}
    for a, b in zip(train, train[1:]):
        tab.setdefault(a, {})
        tab[a][b] = tab[a].get(b, 0) + 1
    maj = max(set(train), key=train.count)
    hit, loss = 0, 0.0
    for a, b in zip(test, test[1:]):
        row = tab.get(a)
        if row:
            pred = max(row, key=row.get)
            prob = (row.get(b, 0) + 1) / (sum(row.values()) + ALPHABET)
        else:
            pred, prob = maj, 1.0 / ALPHABET
        hit += 1 if pred == b else 0
        loss -= math.log2(prob)
    m = max(1, len(test) - 1)
    return hit / m, loss / m


def dwell(seq):
    d = [seq[i + 1][0] - seq[i][0] for i in range(len(seq) - 1)]
    if len(d) < 3:
        return 0.0, 0.0, 0.0
    m = statistics.mean(d)
    sd = statistics.pstdev(d)
    var = sum((x - m) ** 2 for x in d)
    ac = (sum((d[i] - m) * (d[i + 1] - m) for i in range(len(d) - 1)) / var
          if var else 0.0)
    return m, (sd / m if m else 0.0), ac


def boot_ci(values, rng, stat=statistics.mean):
    if len(values) < 3:
        return (float("nan"), float("nan"))
    n = len(values)
    means = []
    for _ in range(BOOT):
        means.append(stat([values[rng.randrange(n)] for _ in range(n)]))
    means.sort()
    return (means[int(0.025 * BOOT)], means[int(0.975 * BOOT)])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="results/entropy_n30")
    ap.add_argument("--out")
    args = ap.parse_args()

    done = {}
    summary = os.path.join(args.dir, "summary.csv")
    if os.path.exists(summary):
        with open(summary) as f:
            for row in csv.DictReader(f):
                if int(row["rc"]) == 0 and int(row["moves"]) > 0:
                    done.setdefault(row["mode"], []).append(int(row["seed"]))

    per_mode = {}
    for mode in ("cycle", "bio", "random"):
        rows = {"moves": [], "h": [], "h_mm": [], "acc": [], "loss": [],
                "cv": [], "acf": []}
        for seed in sorted(done.get(mode, [])):
            path = os.path.join(args.dir, mode, f"seed_{seed}.csv")
            if not os.path.exists(path):
                continue
            seq = [port for _t, port in moves(path)]
            if len(seq) < 20:
                continue
            h, h_mm, _k = entropy(seq)
            acc, loss = held_out(seq)
            _m, cv, acf = dwell(moves(path))
            rows["moves"].append(len(seq))
            rows["h"].append(h)
            rows["h_mm"].append(h_mm)
            rows["acc"].append(acc)
            rows["loss"].append(loss)
            rows["cv"].append(cv)
            rows["acf"].append(acf)
        per_mode[mode] = rows

    rng = random.Random(SEED)
    out = []
    out.append("# n=30 per mode entropy and prediction, regenerated")
    out.append("# command: tools/entropy_analysis.py --out "
               "results/entropy_n30/analysis.txt")
    out.append("")
    for mode in ("cycle", "bio", "random"):
        r = per_mode[mode]
        n = len(r["loss"])
        if n == 0:
            out.append(f"{mode}: no complete parties yet")
            continue
        out.append(f"{mode}: parties {n}")
        for key, label in (("moves", "moves"), ("h", "H bits"),
                           ("h_mm", "H miller-madow"),
                           ("acc", "held-out order-1 accuracy"),
                           ("loss", "log-loss bits per move"),
                           ("cv", "dwell cv"), ("acf", "dwell acf1")):
            m = statistics.mean(r[key])
            lo, hi = boot_ci(r[key], rng)
            out.append(f"  {label}: mean {m:.3f}, 95% bootstrap CI "
                       f"[{lo:.3f}, {hi:.3f}], n={n}")
    out.append("")
    out.append("paired differences, bio minus control, same seed:")
    for control in ("random", "cycle"):
        a = per_mode["bio"]["loss"]
        b = per_mode[control]["loss"]
        k = min(len(a), len(b))
        if k < 3:
            out.append(f"  bio - {control}: not enough complete pairs")
            continue
        diffs = [a[i] - b[i] for i in range(k)]
        m = statistics.mean(diffs)
        lo, hi = boot_ci(diffs, rng)
        out.append(f"  bio - {control} log-loss: mean {m:+.3f} bits, "
                   f"95% bootstrap CI [{lo:+.3f}, {hi:+.3f}], n={k}")
        stronger = sum(1 for d in diffs if d > 0)
        out.append(f"    control harder to predict in {stronger} of {k} seeds")

    text = "\n".join(out) + "\n"
    print(text, end="")
    if args.out:
        with open(args.out, "w") as f:
            f.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
