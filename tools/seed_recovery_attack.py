#!/usr/bin/env python3
"""seed-recovery attack on the DEST_RANDOM schedule (h11).

replays the proteus xorshift64* exactly as src/motor.c does it and searches a
candidate seed range for the one that reproduces the ports observed in a
journal. no-op moves (destination == current port) are not journalled, so the
replay skips them the same way the headless motor does.

usage:
    tools/seed_recovery_attack.py JOURNAL [--lo 0] [--hi 200000] [--k 3]

exit 0 when at least one candidate reproduces the observed prefix.
"""

import argparse
import csv
import sys

MASK = 0xFFFFFFFFFFFFFFFF
MULT = 2685821657736338717
RR_XOR = 0x9E3779B97F4A7C15
PORT_BASE = 40000
PORT_WINDOW = 64
GUARD = 4096


def observed_ports(path):
    """port values in journal order, one entry per realised move.

    accepts a full proteus journal (port in column 8) or a derived
    t_ms,port file (results/research_v2/*_moves.csv).
    """
    ports = []
    prev = None
    full = None
    with open(path, newline="") as f:
        for row in csv.reader(f):
            if not row or row[0].startswith("#") or row[0] == "t_ms":
                continue
            if full is None:
                full = len(row) > 9
            port = int(row[8] if full else row[1])
            if full and prev is None:
                prev = port
                continue
            if prev is None or port != prev:
                ports.append(port)
            prev = port
    return ports


def replay(seed, count):
    """first `count` emitted ports for `seed`, skipping no-op moves."""
    state = (seed ^ RR_XOR) & MASK
    if state == 0:
        state = 88172645463325252
    current = PORT_BASE
    out = []
    for _ in range(GUARD):
        x = state
        x ^= x >> 12
        x ^= (x << 25) & MASK
        x ^= x >> 27
        state = x
        port = PORT_BASE + ((((x * MULT) & MASK)) % PORT_WINDOW)
        if port != current:
            out.append(port)
            current = port
            if len(out) == count:
                break
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("journal")
    ap.add_argument("--lo", type=int, default=0)
    ap.add_argument("--hi", type=int, default=200000)
    ap.add_argument("--k", type=int, default=3)
    args = ap.parse_args()

    observed = observed_ports(args.journal)
    if len(observed) < args.k:
        print(f"journal has {len(observed)} moves, need {args.k}")
        return 2

    target = observed[:args.k]
    candidates = [s for s in range(args.lo, args.hi + 1)
                  if replay(s, args.k) == target]

    print(f"observed prefix ({args.k}): {target}")
    print(f"seed range {args.lo}..{args.hi}: {len(candidates)} candidate(s) "
          f"{candidates[:5]}")
    verified = [s for s in candidates
                if replay(s, len(observed)) == observed]
    for seed in verified[:5]:
        print(f"  seed {seed}: full sequence match = True "
              f"({len(observed)} moves)")
    if candidates and not verified:
        print("  no candidate reproduces the full sequence")
    return 0 if verified else 1


if __name__ == "__main__":
    sys.exit(main())
