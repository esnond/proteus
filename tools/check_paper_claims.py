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
    "results/research_v2/*.md",
    "results/manifest.txt",
)

TOKEN = re.compile(r"\d+(?:[.,]\d+)?")
SKIP_DIRS = {".git", "src/petri"}


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
    return TOKEN.findall(stripped)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("draft")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    root = pathlib.Path.cwd()
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

    if not args.quiet:
        print(f"ok: {len(seen)} distinct numeric tokens, all traceable to the "
              f"repository sources")
    return 0


if __name__ == "__main__":
    sys.exit(main())
