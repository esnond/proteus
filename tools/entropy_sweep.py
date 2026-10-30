#!/usr/bin/env python3
"""night sweep: n=30 per destination mode, 6200 s parties, resumable.

each party is a full proteusd run writing a journal; the sweep keeps four
workers busy, skips files that already exist and are non empty, and appends
one line per finished party to sweep.log. a summary.csv is rewritten after
every completion, so the state of the run is readable at any moment.

usage:
    nohup tools/entropy_sweep.py --seeds 3001..3030 --jobs 4 \
        --out results/entropy_n30 > results/entropy_n30/sweep.out 2>&1 &
"""

import argparse
import concurrent.futures
import csv
import os
import subprocess
import time

MODES = ("cycle", "bio", "random")
SCHEDULE = "tests/fixtures/burst.csv"


def parse_seeds(text):
    if ".." in text:
        lo, hi = text.split("..")
        return list(range(int(lo), int(hi) + 1))
    return [int(x) for x in text.split(",")]


def moves_in(path):
    prev, n = None, 0
    with open(path) as f:
        for line in f:
            if line.startswith("#"):
                continue
            p = line.split(",")
            if len(p) < 10 or p[0] == "t_ms":
                continue
            port = int(p[8])
            if prev is not None and port != prev:
                n += 1
            prev = port
    return n


def run_one(mode, seed, duration, out_dir):
    path = os.path.join(out_dir, mode, f"seed_{seed}.csv")
    log = os.path.join(out_dir, mode, f"seed_{seed}.log")
    if os.path.exists(path) and os.path.getsize(path) > 4096:
        return mode, seed, 0, moves_in(path), "cached"
    t0 = time.time()
    with open(log, "w") as err:
        rc = subprocess.call(
            ["./proteusd", "-s", str(seed), "-d", str(duration),
             "-e", SCHEDULE, "--dest-mode", mode, "-o", path],
            stdout=subprocess.DEVNULL, stderr=err)
    elapsed = time.time() - t0
    if rc != 0:
        return mode, seed, rc, -1, f"failed in {elapsed:.0f}s"
    return mode, seed, rc, moves_in(path), f"ok in {elapsed:.0f}s"


def write_summary(rows, out_dir):
    path = os.path.join(out_dir, "summary.csv")
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["mode", "seed", "rc", "moves", "note"])
        for row in sorted(rows):
            w.writerow(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", default="3001..3030")
    ap.add_argument("--duration", type=int, default=6200)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--out", default="results/entropy_n30")
    args = ap.parse_args()

    seeds = parse_seeds(args.seeds)
    for mode in MODES:
        os.makedirs(os.path.join(args.out, mode), exist_ok=True)

    tasks = [(mode, seed) for mode in MODES for seed in seeds]
    rows = []
    started = time.time()
    with open(os.path.join(args.out, "sweep.log"), "a") as log:
        log.write(f"# start {time.strftime('%Y-%m-%dT%H:%M:%S')} "
                  f"parties={len(tasks)} duration={args.duration} "
                  f"jobs={args.jobs}\n")
        log.flush()
        with concurrent.futures.ThreadPoolExecutor(
                max_workers=args.jobs) as pool:
            futures = {pool.submit(run_one, m, s, args.duration, args.out):
                       (m, s) for m, s in tasks}
            for fut in concurrent.futures.as_completed(futures):
                row = fut.result()
                rows.append(row)
                log.write(f"{time.strftime('%H:%M:%S')} {row[0]} "
                          f"{row[1]} rc={row[2]} moves={row[3]} {row[4]}\n")
                log.flush()
                write_summary(rows, args.out)
        log.write(f"# done {time.strftime('%Y-%m-%dT%H:%M:%S')} "
                  f"elapsed={time.time() - started:.0f}s\n")

    failed = [r for r in rows if r[2] != 0]
    print(f"parties {len(rows)}, failed {len(failed)}, "
          f"elapsed {time.time() - started:.0f}s")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
