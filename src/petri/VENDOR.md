# vendored petri engine

source: https://github.com/esnond/elegans (local working copy
/home/esnond/elegans), mit license, see LICENSE in this directory.

vendored commit: b436adb (2026-10-28).

the engine files are byte identical to the upstream working copy. proteus
consumes the engine through its public headers and state, nothing in
src/petri is edited. two runtime adjustments are made by the proteus
bridge after sim_init, both documented in src/bridge.c:

1. the per neuron membrane noise multiplier is raised from the loader
   default zero to proteus NOISE_SCALE (1.0). assumption, see readme
   limitations.
2. the wall contact edge detectors are primed to the body state, which
   suppresses a phantom ash pulse the engine fires at t 0 because the
   body skeleton is zero initialized inside the contact band.

the connectome is the engine builtin, no csv dataset is vendored.

update procedure: copy the engine sources again, bump the commit here,
rebuild, run make test. the determinism gate compares journals, so an
engine change that alters arithmetic shows up immediately.
