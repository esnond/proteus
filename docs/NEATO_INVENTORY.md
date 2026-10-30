# NEATO_INVENTORY: working-tree inventory, 2026-10-29

owner order: delete every neato mention from the working repository,
robotics lane cancelled (HX-9). history keeps its traces as provenance
(R8), no filter-repo run by the agent.

## method

    grep -rinE 'neato|xv-15|xv15|robot vacuum|robotics|robotic|lidar' \
        --exclude-dir=.git --exclude-dir=results .

independent verification: Explore subagent, same patterns plus a
word-boundary `\bROS\b` pass. listing below is the merged result.

## hits

| pattern | hits | locations |
|---|---|---|
| neato | 0 | none |
| xv-15 / xv15 | 0 | none |
| robot vacuum | 0 | none |
| robotics | 0 | none |
| lidar | 0 | none |
| ROS (word) | 0 | none (substring hits are across, crossover, homeostatic) |

no file or directory name matches *neato*, *xv15*, *robot*, *lidar*.

## near misses, disposition = keep

these are scope exclusions, not a robotics lane. they stay because they
are true and the paper must state what it does not cover.

| file:line | text | disposition |
|---|---|---|
| paper.md:12 | "robot body excluded" | keep, scope limit |
| paper.md:210 | table row "robot body excluded" | keep, scope limit |
| paper.md:258 | "scope cuts (robot excluded)" | keep, scope limit |
| docs/decisions.md:6 | "robot body excluded (AGENTS.md)" | keep, superseded by HX-9 note |

## verdict

neato present: no. scrub is a no-op on the working tree; the cancellation
is recorded as HX-9 in docs/decisions.md so the decision is visible
without rewriting history.
