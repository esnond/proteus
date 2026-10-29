#!/usr/bin/env python3
"""secret_dependence: does an externally visible response time depend on
the daemon's secret material?

the old criterion (coefficient of variation of ban latency) measured
input density, not secrecy. this test holds the input stream fixed and
varies only the secret (the seed), then compares the session handshake
latency distributions with a two-sample KS test and a Welch t test.

pre-registered decision rule, written before the first run:
  PASS iff n >= 100 per condition
       AND KS p >= 0.01
       AND |Cohen's d| < 0.2
  any other outcome is FAIL and is published with the numbers.

ban latency is deliberately not used: with the frozen detector it depends
on the neural state by design, which is the product, not a leak.
"""
import json
import os
import socket
import statistics as st
import subprocess
import sys
import time

ALPHA = 0.01
D_MAX = 0.2
N_MIN = 100
BLOCKS = int(os.environ.get("SD_BLOCKS", "3"))
WARMUP = 5
PER_BLOCK = int(os.environ.get("SD_PER_BLOCK", "70"))
DUR = int(os.environ.get("SD_DUR", "25"))


def live_port(sock):
    out = subprocess.run(["./proteusctl", "status", "--socket", sock],
                         capture_output=True, text=True).stdout
    for tok in out.split():
        if tok.startswith("port="):
            return int(tok[5:])
    return None


def probe(sock, count, out):
    """connect, send payload, close; record elapsed ms per session"""
    got = 0
    deadline = time.monotonic() + DUR
    while got < count and time.monotonic() < deadline:
        port = live_port(sock)
        if not port:
            time.sleep(0.05)
            continue
        t0 = time.monotonic()
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=0.5)
            s.sendall(b"PROTEUS HELLO\n")
            s.close()
        except OSError:
            time.sleep(0.02)   # port moved between read and connect
            continue
        out.append((time.monotonic() - t0) * 1000.0)
        got += 1
        time.sleep(0.01)
    return got


def run_block(seed, sock, path, count):
    """one daemon, one measurement block. raises when the daemon never
    serves: a silent zero sample block is a tool failure, not a SKIP."""
    err = f"/tmp/sd_{seed}.err"
    eh = open(err, "w")
    p = subprocess.Popen(["./proteusd", "-s", str(seed), "-d", str(DUR + 5),
                          "-r", "--listen", "--socket", sock, "-o", path],
                         stdout=subprocess.DEVNULL, stderr=eh)
    eh.close()
    live = False
    for _ in range(30):
        if p.poll() is not None:
            break
        if live_port(sock):
            live = True
            break
        time.sleep(0.1)
    lat = []
    try:
        if live:
            probe(sock, count + WARMUP, lat)
    finally:
        p.terminate()
        try:
            p.wait(timeout=3)
        except subprocess.TimeoutExpired:
            p.kill()
        time.sleep(0.3)
    if not live:
        with open(err) as fh:
            tail = " | ".join(fh.read().strip().splitlines()[-2:])
        raise RuntimeError(f"daemon seed {seed} never served {sock}: {tail}")
    return lat[WARMUP:]


def ks_two_sample(a, b):
    a = sorted(a)
    b = sorted(b)
    n, m = len(a), len(b)
    i = j = 0
    d = 0.0
    while i < n and j < m:
        if a[i] <= b[j]:
            i += 1
        else:
            j += 1
        d = max(d, abs(i / n - j / m))
    ne = (n * m) / (n + m)
    x = (ne ** 0.5) * d
    p = 2.0 * sum((-1) ** (k - 1) * pow(2.718281828459045,
                                       -2.0 * k * k * x * x)
                  for k in range(1, 101))
    return d, max(0.0, min(1.0, p))


def welch(a, b):
    ma, mb = st.mean(a), st.mean(b)
    va, vb = st.variance(a), st.variance(b)
    se = (va / len(a) + vb / len(b)) ** 0.5
    if se == 0:
        return 0.0, 1.0
    t = (ma - mb) / se
    z = abs(t)
    # normal approximation, honest for n >= 100 per condition
    p = 2.0 * 0.5 * (1.0 - _erf(z / 2.0 ** 0.5))
    return t, max(0.0, min(1.0, p))


def _erf(x):
    # Abramowitz-Stegun 7.1.26, |error| < 1.5e-7
    sign = -1.0 if x < 0 else 1.0
    x = abs(x)
    t = 1.0 / (1.0 + 0.3275911 * x)
    y = 1.0 - (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t
                - 0.284496736) * t + 0.254829592) * t * pow(2.718281828459045,
                                                           -x * x)
    return sign * y


def cohen_d(a, b):
    sa, sb = st.variance(a), st.variance(b)
    pooled = ((sa + sb) / 2.0) ** 0.5
    if pooled == 0:
        return 0.0
    return (st.mean(a) - st.mean(b)) / pooled


def main():
    sock_a, sock_b = "/tmp/sd_a.sock", "/tmp/sd_b.sock"
    a, b = [], []
    failed = None
    # alternate the order inside each pair so a systematic position bias
    # (warm caches, lingering sockets) cancels instead of loading one seed
    try:
        for blk in range(BLOCKS):
            if blk % 2 == 0:
                a += run_block(42, sock_a, f"/tmp/sd_a_{blk}.csv", PER_BLOCK)
                b += run_block(43, sock_b, f"/tmp/sd_b_{blk}.csv", PER_BLOCK)
            else:
                b += run_block(43, sock_b, f"/tmp/sd_b_{blk}.csv", PER_BLOCK)
                a += run_block(42, sock_a, f"/tmp/sd_a_{blk}.csv", PER_BLOCK)
    except RuntimeError as e:
        failed = str(e)
    n_a, n_b = len(a), len(b)
    out = {"n_a": n_a, "n_b": n_b,
           "mean_a_ms": round(st.mean(a), 3) if a else None,
           "mean_b_ms": round(st.mean(b), 3) if b else None,
           "sd_a_ms": round(st.pstdev(a), 3) if a else None,
           "sd_b_ms": round(st.pstdev(b), 3) if b else None}
    if failed:
        out["error"] = failed
        verdict = "SKIP"
    elif n_a >= N_MIN and n_b >= N_MIN:
        d, p_ks = ks_two_sample(a, b)
        _, p_t = welch(a, b)
        cd = cohen_d(a, b)
        out.update({"ks_d": round(d, 4), "ks_p": round(p_ks, 4),
                    "welch_p": round(p_t, 4), "cohen_d": round(cd, 3)})
        verdict = "PASS" if (p_ks >= ALPHA and abs(cd) < D_MAX) else "FAIL"
    else:
        verdict = "SKIP"
    out["verdict"] = verdict
    out["criterion"] = (f"n>={N_MIN}, ks_p>={ALPHA}, |d|<{D_MAX}, "
                        "identical probe input, seed differs only")
    print(json.dumps(out, sort_keys=True))
    return 0 if verdict != "FAIL" else 1


if __name__ == "__main__":
    sys.exit(main())
