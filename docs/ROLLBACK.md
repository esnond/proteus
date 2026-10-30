# ROLLBACK.md, exact undo per phase

## r2.5 run (branch r2.5-immune)

phase 0 anchor: tag `r2-bench-anchor` = commit at r2 close (19de9e4).

undo this phase:

    git checkout v1.2-autonomous
    git branch -D r2.5-immune
    git tag -d r2-bench-anchor

if a phase pushed and the push must be undone:

    git push origin :r2.5-immune
    git push origin :refs/tags/r2-bench-anchor

the working tree at the anchor is what the bench validated. no migration
or data format change lands in r2.5 without a line here.

## earlier run (v1.2-autonomous)

    git checkout master
    git branch -D v1.2-autonomous
    git tag -d v1.2-pre-autonomous
    # if push happened:
    git push origin :v1.2-autonomous
    git push origin :v1.2-pre-autonomous
