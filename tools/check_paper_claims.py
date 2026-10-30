#!/usr/bin/env python3
"""honesty gate: every number in a draft paper must exist in the repo sources.

the rule this enforces is the literature-grounding one: a numeric claim in a
paper has to trace to a measurement or a cited source that lives in the
repository. the check is deliberately dumb — it asks only whether each numeric
token in the draft appears anywhere in the source corpus — because a smart
check that misses an invented number is worse than a dumb one that over-reports.

usage:
    tools/check_paper_claims.py paper/DRAFT.md [--quiet]

exit 0 when every numeric token is traceable, 1 otherwise.
"""

import argparse
import pathlib
import re
import sys

SOURCE_GLOBS = (
    "README.md",
    "docs/*.md",
    "results/*.md",
    "results/research_v2/*.md",
    "results/schedule_attack/*.md",
    "results/manifest.txt",
    "results/paper_stats.txt",
)

TOKEN = re.compile(r"\d+(?:[.,]\d+)?")
# file.c:123 is a pointer into the tree, not a numeric claim about a quantity
LINE_REF = re.compile(r"\S+\.[A-Za-z0-9]{1,4}:\d+(?:-\d+)?")
SKIP_DIRS = {".git", "src/petri"}

# a number that exists somewhere in the corpus can still be used with the
# wrong meaning. these pairs bind the critical values to their context; a
# pass is necessary, not sufficient, for a correct claim.
CRITICAL_PAIRS = (
    ("0.139", ("top-1",)),
    ("8705", ("60000",)),
    ("0.145", ("40031",)),
    ("23.4", ("top two", "верхних")),
    ("5.851", ("log-loss",)),
    ("6.025", ("random",)),
    ("0.174", ("bits", "бита")),
    ("5.058", ("order 0",)),
    ("0.0407", ("0.0832",)),
    ("1.519", ("1.321",)),
    ("0.0712", ("accuracy", "точность")),
    ("5.941", ("plug-in",)),
    ("62", ("14/20",)),
    ("145", ("prefix", "префикс")),
    ("212", ("sentinel", "сентинел")),
    ("203", ("5233",)),
    ("200001", ("candidates", "кандидат")),
    ("0.578", ("dop",)),
    ("0.465", ("ash",)),
    ("0.065", ("avb",)),
    ("100/100", ("timing", "тайминг", "сидах")),
    ("0.876", ("second",)),
    ("6.364", ("uniform",)),
)


def corpus_text(root: pathlib.Path) -> str:
    parts = []
    for pattern in SOURCE_GLOBS:
        for path in root.glob(pattern):
            if path.is_file():
                try:
                    parts.append(path.read_text(errors="ignore"))
                except OSError:
                    continue
    return "\n".join(parts)


def numeric_tokens(text: str):
    # code fences are commands, not claims: their numbers need not be sourced
    stripped = re.sub(r"```.*?```", " ", text, flags=re.S)
    stripped = LINE_REF.sub(" ", stripped)
    return TOKEN.findall(stripped)


def pair_check(text: str):
    """require each critical number to sit next to its stated context."""
    missing = []
    for needle, contexts in CRITICAL_PAIRS:
        ok = False
        for m in re.finditer(re.escape(needle), text):
            lo = max(0, m.start() - 160)
            hi = min(len(text), m.end() + 160)
            window = text[lo:hi]
            if any(context in window for context in contexts):
                ok = True
                break
        if not ok:
            missing.append((needle, " or ".join(contexts)))
    return missing


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("draft")
    ap.add_argument("--root", default=".", help="repository root holding the sources")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--pairs", action="store_true",
                    help="also require critical number-context pairs")
    args = ap.parse_args()

    root = pathlib.Path(args.root).resolve()
    draft = pathlib.Path(args.draft)
    if not draft.is_file():
        print(f"FABRICATED_NUMBER: draft missing: {draft}")
        return 2

    corpus = corpus_text(root)
    tokens = numeric_tokens(draft.read_text(errors="ignore"))
    if not tokens:
        print("no numeric tokens found; nothing to check")
        return 0

    missing = []
    seen = set()
    for tok in tokens:
        if tok in seen:
            continue
        seen.add(tok)
        forms = {tok, tok.replace(",", ".")}
        if not any(form in corpus for form in forms):
            missing.append(tok)

    if missing:
        print(f"FABRICATED_NUMBER: {len(missing)} numeric token(s) in "
              f"{draft} are absent from the sources:")
        for tok in sorted(missing, key=lambda t: (len(t), t)):
            print(f"  {tok}")
        print("either cite the source, mark the value UNCERTAIN, or delete "
              "the claim.")
        return 1

    if args.pairs:
        absent = pair_check(draft.read_text(errors="ignore"))
        if absent:
            print(f"CONTEXT_MISSING: {len(absent)} critical number(s) in "
                  f"{draft} appear without the stated context:")
            for needle, context in absent:
                print(f"  {needle} near {context}")
            return 1
        if not args.quiet:
            print(f"ok: {len(CRITICAL_PAIRS)} critical number-context pairs")

    if not args.quiet:
        print(f"ok: {len(seen)} distinct numeric tokens, all traceable to the "
              f"repository sources; a pass is not a proof of correct usage")
    return 0


if __name__ == "__main__":
    sys.exit(main())
