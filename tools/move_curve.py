#!/usr/bin/env python3
# r1 move rate curve: port move frequency against scan density at two
# legitimacy levels. hunger: scans only. fed: scans plus a 1 hz legit
# stream. plus one mixed point (1 hz scans, 1 hz legit). every run is a
# deterministic proteusd session, the verdict table prints at the end.
# the frozen prediction: octopamine high on hunger means more moves per
# minute, serotonin high when fed means fewer, per the r0 motor table.
import subprocess, sys, os

DUR_S = 300
SEEDS = range(1, 11)
DENSITIES = [(0.5, 2000), (1.0, 1000), (2.0, 500), (4.0, 250)]
OUT = "results"


def schedule(rows, path):
    with open(path, "w") as f:
        f.write("# proteus events v2\n# t_ms,kind,amp\n")
        for t, kind, amp in rows:
            f.write(f"{t},{kind},{amp}\n")


def gen(hz, step_ms, legit_hz):
    rows = []
    t = step_ms
    while t <= DUR_S * 1000 - 100:
        rows.append((t, "burst", 1.0))
        t += step_ms
    if legit_hz:
        t = 1000
        while t <= DUR_S * 1000 - 100:
            rows.append((t, "legit", 0.0))
            t += 1000
    rows.sort(key=lambda r: r[0])
    return rows


def run(seed, sched_path, journal_path):
    with open(journal_path, "w") as out:
        rc = subprocess.run(["./proteusd", "-s", str(seed),
                             "-d", str(DUR_S), "-e", sched_path],
                            stdout=out, stderr=subprocess.DEVNULL).returncode
    if rc != 0:
        sys.exit(f"FAILED: proteusd seed {seed} rc={rc}")


def stats(path):
    moves = 0
    ser_late = oct_late = 0.0
    n_late = 0
    for ln in open(path):
        if ln.startswith("#"):
            continue
        f = ln.split(",")
        t = float(f[0])
        if int(f[9]) != 0:
            moves += 1
        if t >= 200000:
            ser_late += float(f[4])
            oct_late += float(f[5])
            n_late += 1
    return moves, ser_late / n_late, oct_late / n_late


def main():
    print(f"# move rate curve, {DUR_S} s per run, equilibrium read from "
          f"the last 100 s, prediction: hunger moves > fed moves")
    print("seed,mode,density_hz,legit_hz,moves,moves_per_min,ser_eq,oct_eq")
    table = {}
    for seed in SEEDS:
        for hz, step in DENSITIES:
            for mode, legit in (("hunger", 0), ("fed", 1)):
                sched = f"/tmp/sched_{mode}_{hz}_{seed}.csv"
                journal = f"{OUT}/mv_{mode}_{hz}_{seed}.csv"
                schedule(gen(hz, step, legit), sched)
                run(seed, sched, journal)
                m, ser, oct_ = stats(journal)
                table[(mode, hz, seed)] = m
                print(f"{seed},{mode},{hz},{legit},{m},{m * 60 / DUR_S:.2f},"
                      f"{ser:.3f},{oct_:.3f}")
        mixed = f"/tmp/sched_mixed_1_{seed}.csv"
        journal = f"{OUT}/mv_mixed_1_{seed}.csv"
        schedule(gen(1.0, 1000, 1), mixed)
        run(seed, mixed, journal)
        m, ser, oct_ = stats(journal)
        print(f"{seed},mixed,1.0,1,{m},{m * 60 / DUR_S:.2f},{ser:.3f},"
              f"{oct_:.3f}")
        print(f"# seed {seed} done", file=sys.stderr)
    print("# paired verdict, hunger vs fed, more moves on hunger")
    for hz, _ in DENSITIES:
        more = sum(1 for s in SEEDS
                   if table[("hunger", hz, s)] > table[("fed", hz, s)])
        print(f"# density {hz} hz: hunger moves higher on {more}/10 seeds")
    print("# DONE")


if __name__ == "__main__":
    main()
