#!/usr/bin/env python3
"""render an animated demo figure from a proteus journal.

pure stdlib: no matplotlib, no PIL, no ffmpeg. the output is an SVG with a
SMIL animation that reveals the traces left to right; without animation it
degrades to a static three-panel plot, which is what a README shows when a
renderer ignores SMIL.

usage:
    tools/render_demo.py JOURNAL [-o docs/demo.svg] [--title TEXT]

panel A: the port timeline (one dot per realised move).
panel B: command interneurons ava/avb and the ash nociceptor trace.
panel C: the matched-filter detector score with the frozen threshold.
"""

import argparse
import csv
import sys

W, H = 940, 560
X0, X1 = 70, 890
A_Y0, A_Y1 = 70, 240
B_Y0, B_Y1 = 275, 385
C_Y0, C_Y1 = 415, 520
PORT_WINDOW = 64
DET_THRESHOLD = 50.0
DET_YMAX = 60.0
ANIM_S = 12
SAMPLES = 600


def read_journal(path):
    rows = []
    with open(path, newline="") as f:
        for row in csv.reader(f):
            if not row or row[0].startswith("#") or row[0] == "t_ms":
                continue
            rows.append({
                "t": float(row[0]) / 1000.0,
                "ava": float(row[1]),
                "avb": float(row[2]),
                "ash": float(row[3]),
                "det": float(row[7]),
                "port": int(row[8]),
            })
    return rows


def panel_x(t, t_end):
    return X0 + (X1 - X0) * (t / t_end if t_end else 0.0)


def polyline(points, y_of, t_end, colour, width):
    pts = " ".join(f"{panel_x(r['t'], t_end):.1f},{y_of(r):.1f}"
                   for r in points)
    return (f'<polyline points="{pts}" fill="none" stroke="{colour}" '
            f'stroke-width="{width}" stroke-linejoin="round"/>')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("journal")
    ap.add_argument("-o", "--out", default="docs/demo.svg")
    ap.add_argument("--title",
                    default="connectome-driven port schedule")
    args = ap.parse_args()

    rows = read_journal(args.journal)
    if not rows:
        print("empty journal")
        return 1
    t_end = rows[-1]["t"]

    span = max(1, len(rows) // SAMPLES)
    sampled = rows[::span]

    moves = []
    prev = rows[0]["port"]
    for r in rows:
        if r["port"] != prev:
            moves.append(r)
            prev = r["port"]
    ports = {r["port"] for r in moves}

    def a_y(r):
        return A_Y1 - (r["port"] % PORT_WINDOW) * (A_Y1 - A_Y0) / (PORT_WINDOW - 1)

    def b_y(r):
        v = min(1.0, max(0.0, r["ava"]))
        return B_Y1 - v * (B_Y1 - B_Y0)

    def b_y2(r):
        v = min(1.0, max(0.0, r["avb"]))
        return B_Y1 - v * (B_Y1 - B_Y0)

    def b_y3(r):
        v = min(1.0, max(0.0, r["ash"]))
        return B_Y1 - v * (B_Y1 - B_Y0)

    def c_y(r):
        v = min(DET_YMAX, max(0.0, r["det"]))
        return C_Y1 - v * (C_Y1 - C_Y0) / DET_YMAX

    dots = "".join(
        f'<circle cx="{panel_x(r["t"], t_end):.1f}" cy="{a_y(r):.1f}" '
        f'r="3" fill="#2f6feb"/>' for r in moves)

    out = []
    out.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
        f'viewBox="0 0 {W} {H}" font-family="monospace">')
    out.append(f'<rect width="{W}" height="{H}" fill="#0d1117"/>')
    out.append(f'<text x="{X0}" y="32" fill="#c9d1d9" font-size="17">'
               f'{args.title}</text>')
    out.append(f'<text x="{X0}" y="54" fill="#8b949e" font-size="12">'
               f'{t_end:.0f} s model time · {len(moves)} port moves · '
               f'{len(ports)} distinct ports</text>')

    for name, y0, y1, label in (
            ("A", A_Y0, A_Y1, "port index"),
            ("B", B_Y0, B_Y1, "ava / avb / ash"),
            ("C", C_Y0, C_Y1, "detector score")):
        out.append(f'<rect x="{X0}" y="{y0}" width="{X1 - X0}" '
                   f'height="{y1 - y0}" fill="none" stroke="#30363d"/>')
        out.append(f'<text x="{X0 - 8}" y="{y0 + 12}" fill="#8b949e" '
                   f'font-size="11" text-anchor="end">{label}</text>')
        out.append(f'<text x="{X0 - 8}" y="{y1}" fill="#484f58" '
                   f'font-size="10" text-anchor="end">{y0}</text>')

    out.append('<defs><clipPath id="reveal">'
               f'<rect x="{X0}" y="{A_Y0 - 4}" width="0" '
               f'height="{C_Y1 - A_Y0 + 8}">'
               f'<animate attributeName="width" values="0;{X1 - X0}" '
               f'dur="{ANIM_S}s" repeatCount="indefinite"/>'
               '</rect></clipPath></defs>')

    out.append('<g clip-path="url(#reveal)">')
    out.append(dots)
    out.append(polyline(sampled, b_y, t_end, "#3fb950", 1.2))
    out.append(polyline(sampled, b_y2, t_end, "#d29922", 1.2))
    out.append(polyline(sampled, b_y3, t_end, "#a371f7", 1.0))
    out.append(polyline(sampled, c_y, t_end, "#f85149", 1.2))
    out.append('</g>')

    thr_y = C_Y1 - DET_THRESHOLD * (C_Y1 - C_Y0) / DET_YMAX
    out.append(f'<line x1="{X0}" y1="{thr_y:.1f}" x2="{X1}" y2="{thr_y:.1f}" '
               f'stroke="#8b949e" stroke-dasharray="4 4" stroke-width="1"/>')
    out.append(f'<text x="{X1 - 4}" y="{thr_y - 5:.1f}" fill="#8b949e" '
               f'font-size="10" text-anchor="end">threshold '
               f'{DET_THRESHOLD:.0f}</text>')

    out.append(f'<line x1="{X0}" y1="{A_Y0 - 4}" x2="{X0}" '
               f'y2="{C_Y1 + 4}" stroke="#58a6ff" stroke-width="2">'
               f'<animate attributeName="x1" values="{X0};{X1}" '
               f'dur="{ANIM_S}s" repeatCount="indefinite"/>'
               f'<animate attributeName="x2" values="{X0};{X1}" '
               f'dur="{ANIM_S}s" repeatCount="indefinite"/></line>')

    out.append(f'<text x="{X0}" y="{H - 16}" fill="#8b949e" font-size="11">'
               'blue = port move · green = ava · yellow = avb · '
               'purple = ash · red = detector</text>')
    out.append('</svg>')

    with open(args.out, "w") as f:
        f.write("\n".join(out) + "\n")
    print(f"{args.out}: {len(moves)} moves, {len(ports)} distinct ports, "
          f"{t_end:.0f} s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
