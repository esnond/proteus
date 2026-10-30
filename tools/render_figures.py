#!/usr/bin/env python3
"""render the paper figures as plain svg, stdlib only.

every number below is copied from results/paper_stats.txt,
results/schedule_attack/REPORT.md or src/config.h; the honesty gate
traces them. figures land in docs/figures/.
"""

import os

OUT = "docs/figures"

INK = "#1a1a2e"
MUTE = "#8d99ae"
BIO = "#2a9d8f"
CYC = "#e76f51"
RND = "#457b9d"
GRID = "#e5e9f0"
ACC = "#e63946"


def head(w, h, title):
    return [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" role="img">',
            f'<title>{title}</title>',
            '<style>text{font-family:sans-serif;fill:' + INK + '}</style>',
            f'<rect width="{w}" height="{h}" fill="white"/>']


def save(name, lines):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(path)


def box(x, y, w, h, label, fill="#f8f9fa", stroke=INK, fs=12):
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>' +
            ''.join(f'<text x="{x + w / 2}" y="{y + h / 2 - 4 + i * 15}" '
                    f'text-anchor="middle" font-size="{fs}">{t}</text>'
                    for i, t in enumerate(label)))


def arrow(x1, y1, x2, y2, label="", color=INK):
    return (f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" '
            f'stroke="{color}" stroke-width="1.5" marker-end="url(#ah)"/>' +
            (f'<text x="{(x1 + x2) / 2}" y="{(y1 + y2) / 2 - 6}" '
             f'text-anchor="middle" font-size="10" fill="{MUTE}">{label}</text>'
             if label else ""))


def bars(name, w, h, groups, ymax, ylab, note, ref=None, reftxt=""):
    """groups: list of (label, value, color)."""
    L = head(w, h, name)
    L.append('<defs><marker id="ah" markerWidth="8" markerHeight="8" '
             'refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" '
             f'fill="{INK}"/></marker></defs>')
    x0, y0, x1, y1 = 70, h - 60, w - 30, 40
    for i in range(5):
        gy = y0 - (y0 - y1) * i / 4
        L.append(f'<line x1="{x0}" y1="{gy:.0f}" x2="{x1}" y2="{gy:.0f}" '
                 f'stroke="{GRID}"/>')
        L.append(f'<text x="{x0 - 8}" y="{gy + 4:.0f}" text-anchor="end" '
                 f'font-size="10" fill="{MUTE}">{ymax * i / 4:g}</text>')
    L.append(arrow(x0, y0, x0 - 12, y1 + 10))
    L.append(f'<text x="{x0 - 58}" y="{y1 + 12}" font-size="10" '
             f'fill="{MUTE}" transform="rotate(-90 {x0 - 58} {y1 + 12})">{ylab}</text>')
    n = len(groups)
    bw = (x1 - x0) / (n + 1) * 0.55
    for i, (lab, val, col) in enumerate(groups):
        cx = x0 + (x1 - x0) * (i + 1) / (n + 1)
        bh = (y0 - y1) * val / ymax
        L.append(f'<rect x="{cx - bw / 2:.0f}" y="{y0 - bh:.0f}" '
                 f'width="{bw:.0f}" height="{bh:.0f}" fill="{col}"/>')
        L.append(f'<text x="{cx:.0f}" y="{y0 - bh - 6:.0f}" text-anchor="middle" '
                 f'font-size="11">{val:g}</text>')
        L.append(f'<text x="{cx:.0f}" y="{y0 + 16}" text-anchor="middle" '
                 f'font-size="11">{lab}</text>')
    if ref is not None:
        ry = y0 - (y0 - y1) * ref / ymax
        L.append(f'<line x1="{x0}" y1="{ry:.0f}" x2="{x1}" y2="{ry:.0f}" '
                 f'stroke="{ACC}" stroke-dasharray="6 4" stroke-width="1.5"/>')
        L.append(f'<text x="{x1 - 4}" y="{ry - 6:.0f}" text-anchor="end" '
                 f'font-size="10" fill="{ACC}">{reftxt}</text>')
    L.append(f'<text x="{x0}" y="{h - 12}" font-size="10" fill="{MUTE}">{note}</text>')
    return L + ["</svg>"]


def fig_architecture():
    w, h = 880, 400
    L = head(w, h, "proteus data flow")
    L.append('<defs><marker id="ah" markerWidth="8" markerHeight="8" '
             'refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" '
             f'fill="{INK}"/></marker></defs>')
    L.append(box(20, 40, 170, 90, ["scan events csv", "tests/fixtures/burst.csv",
                                   "ash envelopes + oct/ser"]))
    L.append(box(250, 30, 200, 110, ["petri engine, c99", "279 neurons, varshney",
                                     "deterministic, seeded", "1.25 ms/tick"], "#eaf4f4", BIO))
    L.append(box(510, 30, 170, 110, ["journal v2", "t_ms, port, cause,", "det_score, events"], "#eaf4f4", BIO))
    L.append(box(510, 210, 170, 90, ["motor", "dest: cycle / bio / random", "xorshift64* seed"]))
    L.append(box(740, 210, 120, 90, ["tcp listener", "+ nftables bans"]))
    L.append(box(250, 240, 200, 80, ["detector", "matched filter on avb,", "threshold 50.0"]))
    L.append(box(20, 260, 170, 60, ["attacker", "scan burst"]))
    L.append(arrow(190, 85, 248, 85, "event stream"))
    L.append(arrow(450, 85, 508, 85, "ticks"))
    L.append(arrow(595, 140, 595, 208, "replay"))
    L.append(arrow(450, 280, 508, 270, "det_fired"))
    L.append(arrow(680, 255, 738, 255, "move / ban"))
    L.append(arrow(190, 290, 248, 285, "scan"))
    L.append(f'<text x="440" y="{h - 14}" font-size="10" fill="{MUTE}">'
             'headless path: journal only, no sockets; live path diverges (src/motor.c)</text>')
    return L + ["</svg>"]


def fig_entropy():
    g = [("cycle", 6.481, CYC), ("bio", 3.829, BIO), ("random", 5.687, RND)]
    return bars("miller-madow entropy of the port label, n=1 recon",
                560, 340, g, 8, "bits, H_mm",
                "seed 1001, 600 s, tools/paper_stats.py; uniform 64 symbols = 6 bits")


def fig_accuracy():
    g = [("cycle", 0.531, CYC), ("bio", 0.185, BIO), ("random", 0.000, RND)]
    return bars("held out order 1 accuracy, n=1 recon",
                560, 340, g, 0.9, "accuracy",
                "seed 1001; add 1 smoothing, 64 symbols; dashed: uniform 0.0156",
                ref=0.0156, reftxt="uniform 0.0156")


def fig_detection():
    w, h = 720, 260
    L = head(w, h, "the detector fires before the stimulus")
    L.append('<defs><marker id="ah" markerWidth="8" markerHeight="8" '
             'refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" '
             f'fill="{INK}"/></marker></defs>')
    y = 150
    L.append(f'<line x1="60" y1="{y}" x2="660" y2="{y}" stroke="{INK}" '
             'stroke-width="2" marker-end="url(#ah)"/>')
    for i in range(0, 8):
        x = 60 + i * 85
        L.append(f'<line x1="{x}" y1="{y}" x2="{x}" y2="{y + 8}" stroke="{MUTE}"/>')
        L.append(f'<text x="{x}" y="{y + 26}" text-anchor="middle" font-size="10" '
                 f'fill="{MUTE}">{i} s</text>')
    L.append(box(85, y - 80, 150, 44, ["det_fired, all 10 journals", "t = 1000 ms"], "#fdecea", ACC))
    L.append(box(300, y - 80, 170, 44, ["burst stimulus", "t = 2000 ms"], "#eaf4f4", BIO))
    L.append(box(430, y + 42, 190, 44, ["first ash response", "t = 2010 ms"], "#eaf4f4", BIO))
    L.append(f'<text x="60" y="{h - 14}" font-size="10" fill="{MUTE}">'
             '10 of 10 burst journals record det_fired before the burst; '
             '0 of 10 fire within 3 s after the stimulus (results/schedule_attack, cal_v12)</text>')
    return L + ["</svg>"]


def fig_moves():
    w, h = 560, 340
    g = [("mean", 97.5, BIO)]
    L = bars("quiet move rate against the registered band", w, h, g,
             320, "moves per 600 s",
             "n = 10, sd 24.9, t interval [79.7, 115.3]; registered band 100..300",
             ref=None, reftxt="")
    x0, y0, x1, y1 = 70, h - 60, w - 30, 40
    sx = lambda v: x0 + (x1 - x0) * v / 320.0
    sy = lambda v: y0 - (y0 - y1) * v / 320.0
    extra = []
    a, b, col, lab = 100, 300, ACC, "registered band 100..300"
    extra.append(f'<rect x="{sx(a):.0f}" y="{y1 + 6}" width="{sx(b) - sx(a):.0f}" '
                 f'height="{y0 - y1 - 6}" fill="{col}" opacity="0.08"/>')
    extra.append(f'<text x="{sx((a + b) / 2):.0f}" y="{y1 + 24}" text-anchor="middle" '
                 f'font-size="10" fill="{col}">{lab}</text>')
    extra.append(f'<line x1="{sx(97.5):.0f}" y1="{sy(79.7):.0f}" x2="{sx(97.5):.0f}" '
                 f'y2="{sy(115.3):.0f}" stroke="{INK}" stroke-width="2"/>')
    for v in (79.7, 115.3):
        extra.append(f'<line x1="{sx(97.5) - 8:.0f}" y1="{sy(v):.0f}" x2="{sx(97.5) + 8:.0f}" '
                     f'y2="{sy(v):.0f}" stroke="{INK}" stroke-width="2"/>')
    return L[:-1] + extra + [L[-1]]


if __name__ == "__main__":
    save("architecture.svg", fig_architecture())
    save("entropy.svg", fig_entropy())
    save("accuracy.svg", fig_accuracy())
    save("detection.svg", fig_detection())
    save("moves.svg", fig_moves())
