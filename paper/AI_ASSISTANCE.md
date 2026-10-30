# declaration of ai assistance

this declaration follows the disclosure practice of the icmje
recommendations and the elsevier and nature editorial policies: ai
assistance is disclosed in a dedicated statement, ai tools are not
authors, and a human author is accountable for the whole work.

## what the ai tools were used for

- editing and restructuring documentation and paper text, style unification;
- auxiliary analysis and rendering scripts under `tools/` (statistics
  recomputation, figure rendering, sweep tooling);
- boilerplate and test scaffolding in the proteus control plane;
- two adversarial review passes over the paper draft, which produced the
  retraction and correction list recorded in `paper/FINAL.md`.

## what the ai tools were not used for

- the petri connectome engine (`src/petri/` in the upstream repository)
  was written by the author from scratch;
- no ai tool designed experiments, chose thresholds or interpreted
  results: the detector threshold 50.0 was frozen by a pre registered
  procedure on seeds 101..110 before the confirmatory runs;
- no ai tool produced or altered measurement data; every journal was
  written by the `proteusd` binary and is hashed in the release manifest;
- no ai tool performed peer review of this repository for publication.

## verification

every numeric claim in `paper/FINAL.md` is checked by a mechanical gate
(`tools/check_paper_claims.py`): a number that does not exist in the
repository sources fails the build. documentation changes pass a second
mechanical style gate. the human author ran both gates on every revision
and reviewed the diffs.

## accountability

the author is the single accountable person for all content, including
everything the ai tools produced. contact and citation data are in
`CITATION.cff` and `.zenodo.json`.
