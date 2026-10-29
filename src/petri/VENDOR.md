# vendored petri engine

source: https://github.com/esnond/elegans (local working copy
/home/esnond/elegans), mit license, see LICENSE in this directory.

vendored commit: 42d5062 (2026-10-29, the v1.2 experiment line; v1.0.1
is the last tagged release upstream). this is the full varshney wiring
core: 279 somatic neurons from data/varshney_full.csv (vendored at
src/petri/data/, sha256 7aa6fadac02e4ede...), avb slow command layer,
eight alplm muscles. upstream v1.0.1 errata (phantom pulse, noise_scale
data path) are part of this tree.

changed files vs the previous vendor point (b436adb): sim.c, sim.h,
neuron.c, config.h; everything else stayed byte identical.

proteus consumes the engine through its public headers and state,
nothing in src/petri is edited. one runtime adjustment remains, done
by the proteus bridge after sim_init, documented in src/bridge.c:

1. the per neuron membrane noise multiplier is raised from the loader
   default zero to proteus NOISE_SCALE (1.0). assumption, see readme
   limitations. h10 check: the ava saturation under varshney is the
   wiring regime, not noise (upstream pegs ava at shipped noise 0 too).

the r0 wall-contact priming adjustment is REMOVED at this vendor bump:
it no longer suppresses the t0 ash transient on v1.2 (measured with
priming intact), the transient decays inside the engine tau and the
detector baseline absorbs it.

the connectome is the varshney csv above; proteus loads it by
VARSHNEY_CSV relative to the working directory and fails the start
loudly when it is missing (no silent legacy fallback). the engine
builtin wiring remains available upstream but is not used by proteus.

update procedure: copy the engine sources again, bump the commit here,
rebuild, run make test. the determinism gate compares journals, so an
engine change that alters arithmetic shows up immediately.
