# bulk data archive

the git repository carries code, tools, small analysis artifacts and the
release manifest. the bulk measurement data (about 1.3 GB) left the git
tree after v1.0.3 and is intended for a zenodo archive. the doi is
pending, the archive is not yet publicly available, and no artifact
reproducibility claim is made until the doi resolves. the h9 journals are
still present in this working tree, so the paper tables are checkable
here; that is not the same as an archived artifact.

## what is in the archive

| path | size | content |
|---|---|---|
| results/journals_v12/{bio,cycle,random}/seed_*.csv | 300 files, 1.2 GB | h9 journals, 600 s per party, 100 seeds x 3 modes |
| results/journals_v12/*/seed_*.log | 300 files | per party stderr |
| results/cal_v12/{fa,det_burst}_101..110.csv | 20 files, 79 MB | detector calibration: false alarm and burst journals, seeds 101..110 |
| results/entropy_n30/{bio,cycle,random}/seed_*.csv | 90 files, in progress | n=30 sweep, 6200 s per party |

small per mode summaries stay in git: `results/journals_v12/*/run_summary.txt`
(records the binary cafd37d7 at commit 4c5eb5d that produced the h9
journals), and the sweep summary `results/entropy_n30/summary.csv`.

## integrity

`results/manifest.txt` (732 lines, 724 sha256 entries, commit 91cabe5a,
binary 60b04860...) contains the sha256 of every archived h9 journal and
calibration file. a `sha256sum -c` run after the revision 5 edits reports
697 OK, 11 mismatched and 16 missing entries; all 300 h9 journal csv files
are among the OK entries. the 11 mismatches are docs/CLOSE.md,
docs/PLAN_V11.md, docs/RESEARCH_V2.md, docs/STATE.md,
docs/hypotheses_results.md, results/manifest.txt, results/paper_stats.txt,
results/schedule_attack/REPORT.md, tools/check_paper_claims.py,
tools/paper_stats.py and tools/seed_recovery_attack.py. the missing files
are docs removed before the archive split. the archive files can be
verified against the manifest after download, but the manifest is not
regenerated and the doi is pending.

## reproduction

to recompute any paper table locally, fetch the archive and place the
three directories back under `results/`; all commands in paper/FINAL.md
assume that layout. the sweep reproduces from scratch with
`tools/entropy_sweep.py`, the analysis with `tools/entropy_analysis.py`,
and every cited number with `tools/paper_stats.py`.
