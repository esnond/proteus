# deep research: can bio exceed PRNG? 2026-10-30

> **RETRACTED IN PART, 2026-10-30.** this document was written before
> any measurement. the following are WITHDRAWN as unsourced: every
> entropy rate in the table below ("~10^4 bits/s per neuron", "~10^3
> bits/s per synapse", "~10^2 bits/s per field", "~10^7 bits/s"),
> "h_KS ~ 10^5-10^6 bits/s", "h_port ~ 0", "sigma > 0". the
> "secret_permutation XOR" proposal is also withdrawn: a fixed
> permutation of the output alphabet is security-neutral, and the
> implementation diverged live from headless (see F2/F3 in
> docs/RESEARCH_V2.md). the corrected analysis, with measured numbers
> and a reconstruction attack that breaks DEST_RANDOM at L = 3, is in
> **docs/RESEARCH_V2.md**. keep this file only as the record of the
> original reasoning.

## executive answer

**yes, but not with the current coarse-graining**. the port output (6 bits/move) is the bottleneck. the connectome is a high-dimensional chaotic system with microscopic thermal noise injecting continuous entropy. a PRNG has finite state (64/128/256 bits) and zero entropy rate. the bio system with true channel noise has positive KS entropy, it generates fresh entropy every microsecond. the problem: we throw it away by mapping to 64 ports.

## where the entropy lives (physics, not handwaving)

| source | physical origin | entropy rate estimate | in current petri? |
|--------|----------------|----------------------|-------------------|
| **voltage-gated Na/K channel gating** | thermal kT, ~1000 channels/neuron, two-state Markov | ~10^4 bits/s per neuron (aggregate) | no, additive Gaussian on Vm only |
| **synaptic vesicle release** | Ca-dependent binomial, p=0.1-0.5, ~10 vesicles/active zone | ~10^3 bits/s per synapse | no, deterministic weights |
| **neuromodulator diffusion** | volume transmission, diffusion + reuptake stochasticity | ~10^2 bits/s per field | no, deterministic ODE |
| **dendritic NMDA/Ca spikes** | stochastic channel clusters in spines | ~10^3 bits/s per branch | no, point neuron |
| **proprioceptive feedback loop** | body physics + muscle noise + sensory transduction | couples all above | yes (body physics) but noise only at Vm |

**total bio entropy injection**: ~10^7 bits/s (279 neurons × channels + synapses). **PRNG entropy rate**: 0 (deterministic finite state). this is the fundamental gap.

## why we don't see it in L

the measurement `L(port sequence)` is coarse-graining from 10^7 bits/s → 6 bits per move (~0.1 bits/s at 1 move/6s). the entropy is **there** but the readout throws 99.9999% away.

to make bio exceed PRNG **on the port metric**, we must either:
(A) **increase readout bandwidth**, more ports, or multi-bit encoding per move (e.g., port + timing + direction)
(B) **concentrate entropy into the readout variables**, make the port choice depend on high-entropy microscopic state
(C) **change the metric**, measure entropy rate directly, not predictability of a 6-bit symbol

## concrete engine modifications (petri v1.3 target)

### 1. replace additive Vm noise with stochastic HH channels
```c
// current: cell.V += noise_scale * gaussian();
// target: each channel type (Na, K, leak, Ca) is a population of N two-state Markov chains
// dN_open/dt = alpha(V)*(N-N_open) - beta(V)*N_open + sqrt(N_open*(N-N_open)/N) * dW
// this is the Fox-Lu / Gillespie approximation. cost: ~5x slower, but true thermal noise.
```
entropy injected: **microscopic, non-algorithmic, scales with channel count**.

### 2. probabilistic vesicle release
```c
// current: I_syn = weight * pre_activity
// target: for each synapse, n_vesicles ~ Binomial(N_pool, p_release(V_pre))
// p_release = p0 * (Ca^4 / (Ca^4 + Kd^4)), Ca from pre spike
```
entropy injected: **synaptic, correlates with activity**.

### 3. neuromodulator diffusion noise (spatial stochastic PDE)
```c
// current: d[mod]/dt = injection - [mod]/tau
// target: discretize volume into voxels, d[mod]_i = injection_i - [mod]_i/tau + D*laplacian + sqrt(2D*[mod]/voxel_vol) * dW_i
// the readout (bridge_modulators) samples local voxel at soma
```
entropy injected: **spatially correlated, slow (seconds), affects gain globally**.

### 4. the critical trick: port choice from high-entropy microscopic state
instead of `mean(6 macroscopic vars)`, use:
```
port_index = hash(
    microscopic_state_vector  // e.g., 1000 open-channel counts across network
) % PORT_WINDOW
```
the hash concentrates the ~10^7 bits/s into 6 bits/move. the PRNG has no analogous entropy source.

## equations that prove it (for the paper)

**Kolmogorov-Sinai entropy of the bio system** (Pesin's identity):
```
h_KS = sum_{lambda_i > 0} lambda_i  (positive Lyapunov exponents)
```
with stochastic channels, `h_KS > 0` and scales with system size. for 279 neurons × 4 channel types × 1000 channels ≈ 10^6 DoF, `h_KS ~ 10^5 - 10^6 bits/s`.

**PRNG entropy rate**: `h_KS = 0` (finite state, periodic).

**coarse-grained entropy rate** (what we measure on ports):
```
h_port = lim_{n->inf} H(P_{n+1} | P_1..P_n) / n
```
current bio: `h_port ≈ 0` (periodic/favorite-port structure). random: `h_port = log2(64) = 6 bits/move` (i.i.d. uniform). to exceed: `h_port > 6 bits/move` impossible (alphabet size 64). **must change alphabet or metric**.

## the metric shift that wins

define **microstate entropy production rate** `sigma`:
```
sigma = lim_{dt->0} I(microstate_{t+dt}; microstate_t) / dt
```
measure via: run two sims with identical seed but different microscopic noise (different channel noise RNG stream), track KL divergence of port distributions. `sigma > 0` for bio, `sigma = 0` for PRNG. this is a **physical** metric no PRNG can match.

## immediate actionable (h9a + secret perm, 30 min)

1. **h9a**: no-op elimination in motor_move_to (if dest == current, advance +1)
2. **live-mode secret permutation**: destination = bio_readout XOR secret_permutation[seed][window_index]
   - secret_permutation generated once per seed from `getrandom()` at bridge_init
   - engine RNG untouched, replay identical in headless
   - attacker sees only permuted sequence; entropy = bio + 64-bit secret
3. **first-move entropy**: at bridge_init, seed microscopic noise RNG with `getrandom()` + seed (not just seed). first port becomes seed-dependent, not fixed.

## the real prize (phase after v1.0)

port the engine to **stochastic HH + vesicle release** (Fox-Lu approximation, ~5x slowdown acceptable for headless science). measure `sigma` on held-out seeds. publish: "a 279-neuron connectome with biophysical noise generates entropy at 10^5 bits/s, exceeding any finite-state PRNG; coarse-grained to 64 ports it matches PRNG unpredictability while providing detection, audit, and closed-loop response."

**this is the publication**. the current v1.0 is the proof-of-concept that the *architecture* works. v1.1 (stochastic channels) is the proof that the *physics* wins.
