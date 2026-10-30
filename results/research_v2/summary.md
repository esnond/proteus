# research_v2: measured move sequences

source: `./proteusd -s 1001 -d 600 -e tests/fixtures/burst.csv --dest-mode MODE -o /tmp/MODE_1001.csv`
binary sha256 `60b0486045893880…`, commit `e58a92c`, headless.
each file lists the realised moves only (t_ms of the port change, port).
full reports: docs/RESEARCH_V2.md, docs/hypotheses_results.md (h11).

| mode | moves | distinct | H(port) bits | held-out order-1 | dwell med / CV / acf1 | L |
|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 0.531 (1.000 on states seen) | 2800 ms / 1.29 / -0.11 | 65 |
| bio | 69 | 16 | 3.672 | 0.185 | 3830 ms / 1.18 / -0.09 | 0.90 not reached |
| random | 80 | 45 | 5.290 | 0.000 | 2800 ms / 1.30 / -0.11 | **3** |

reconstruction attack (h11):

    tools/seed_recovery_attack.py random_1001_moves.csv --k 3
    -> 1 candidate(s) [1001], full sequence match = True

extract a move sequence from a full journal:

    python3 - <<'PY'
    import csv
    prev=None
    for row in csv.reader(open("JOURNAL")):
        if not row or row[0].startswith("#") or row[0]=="t_ms": continue
        p=int(row[8])
        if prev is None: prev=p; continue
        if p!=prev: print(row[0], p); prev=p
    PY
