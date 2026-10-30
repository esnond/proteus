#!/usr/bin/env python3
"""numeric parity check between paper/FINAL.md and paper/FINAL_RU.md.

the two versions are allowed to use different words, not different numbers.
the check extracts numeric tokens outside code fences and line references,
normalizes the decimal comma to a dot, and reports tokens that appear in one
version but not the other. it also requires a fixed list of key values to be
present in both, so a token that is dropped from one version is caught even
when the other version still has it.

usage:
    tools/check_paper_parity.py [--en paper/FINAL.md] [--ru paper/FINAL_RU.md]

exit 0 when the numeric sets match and every key value is present, 1 otherwise.
"""

import argparse
import pathlib
import re
import sys

TOKEN = re.compile(r"\d+(?:[.,]\d+)?")
LINE_REF = re.compile(r"\S+\.[A-Za-z0-9]{1,4}:\d+(?:-\d+)?")
KEY_VALUES = (
    "5.851", "4.869", "6.025", "0.221", "0.129", "0.174",
    "5.058", "6.306", "6.364", "6.000",
    "4.014", "5.468", "0.0712", "0.0100", "0.0407", "0.0832",
    "1.519", "1.321", "5.941", "5.931", "5.952",
    "62", "14/20", "145", "156", "212", "300",
    "203", "5233", "6418", "8705", "60000", "0.145", "0.139",
    "23.4", "0.578", "0.465", "0.065", "200001", "100/100",
)


def tokens(path: pathlib.Path):
    """measurement-like tokens: decimals and integers of three or more digits.

    short integers are section numbers, counts and line ranges, not results.
    """
    text = path.read_text(errors="ignore")
    text = re.sub(r"```.*?```", " ", text, flags=re.S)
    text = LINE_REF.sub(" ", text)
    keep = set()
    for tok in TOKEN.findall(text):
        tok = tok.replace(",", ".")
        if ("." in tok and len(tok) >= 3) or (tok.isdigit() and len(tok) >= 3):
            keep.add(tok)
    return keep


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--en", default="paper/FINAL.md")
    ap.add_argument("--ru", default="paper/FINAL_RU.md")
    args = ap.parse_args()
    en_path = pathlib.Path(args.en)
    ru_path = pathlib.Path(args.ru)
    if not en_path.is_file() or not ru_path.is_file():
        print("PARITY_MISSING: one of the paper files is missing")
        return 2

    en = tokens(en_path)
    ru = tokens(ru_path)
    en_text = en_path.read_text(errors="ignore")
    ru_text = ru_path.read_text(errors="ignore")
    only_en = sorted(en - ru, key=lambda t: (len(t), t))
    only_ru = sorted(ru - en, key=lambda t: (len(t), t))
    key_missing = [k for k in KEY_VALUES
                   if k not in en_text or k not in ru_text]

    if only_en:
        print(f"PARITY_NUMBER: {len(only_en)} token(s) only in {en_path}:")
        for t in only_en:
            print(f"  {t}")
    if only_ru:
        print(f"PARITY_NUMBER: {len(only_ru)} token(s) only in {ru_path}:")
        for t in only_ru:
            print(f"  {t}")
    if key_missing:
        print(f"PARITY_KEY: {len(key_missing)} key value(s) missing:")
        for t in key_missing:
            print(f"  {t}")
    if only_en or only_ru or key_missing:
        return 1

    print(f"ok: {len(en)} numeric tokens, identical sets; "
          f"{len(KEY_VALUES)} key values present in both files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
