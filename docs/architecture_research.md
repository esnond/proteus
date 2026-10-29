# Architecture research: the tick, idle CPU, and industry patterns

date: 2026-10-29, r3 phase A. inputs: gprof of 30k ticks, poor-man
backtrace sampling, strace -c, web literature pass (sources inline).

## measured baseline

- 464 us per simulation tick headless (200k ticks, 92.9 s wall).
- gprof: body_step 58.6% (worm body physics, vendored petri engine),
  sim_step 15.7%, world_step 5.8%, neuron_act 3.7% (156 calls/tick),
  rng_normal 3.4% (970 calls/tick), nervous_hist 2.5% (413 calls/tick).
- syscalls are not the cost: 2002 writes per 2000 ticks (the journal is
  line buffered), 0.004% of runtime.
- conclusion: a 20x engine speedup is not available without touching the
  floating point order, which the determinism gate forbids. the tick
  cost is a constant of the product.

## how production systems solve "fixed logical rate vs idle cpu"

1. Fixed-timestep accumulator with a clamp (game loops): accumulate wall
   time, step once per dt of it, and CLAMP the accumulator so a wake
   never catches up unboundedly ("spiral of death" guard). The logical
   clock runs slow whenever the loop is idle; bursts bring catch-up
   bounded by the clamp.
   [Fiedler, Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep)
   [GameDev SE](https://gamedev.stackexchange.com/questions/187660/fixed-timestep-game-loop-why-interpolation)
2. Discrete-event simulation advances the clock to the next scheduled
   event; between events nothing runs. Correct for event-sparse models,
   wrong for a controller whose stimulus statistics are defined per
   tick: proteus tried a pure event-driven loop and the detector broke
   (det_score 6.25 with 121/134 ticks carrying input; the calibrated
   regime is ~6-8% of ticks carrying input).
3. Tickless kernels stop the periodic tick when idle but keep scheduled
   timers; work with deadlines runs when its timer fires. Maps to: the
   token reaper owns a coarse deadline, the simulation is not a deadline
   worker.

## what published MTD systems do

- OF-RHM mutates host IPs from an SDN controller; the schedule generator
  is decoupled from the data plane and its cost is negligible.
  [OF-RHM, HotSDN 2012](http://conferences.sigcomm.org/sigcomm/2012/paper/hotsdn/p127.pdf)
- MT6D hops IPv6 addresses per session; the mutation loop is a userspace
  daemon with trivial CPU. [MT6D in 6LoWPAN](https://augusta.elsevierpure.com/en/publications/implementing-moving-target-ipv6-defense-to-secure-6lowpan-in-the-)
- Surveys (IP hopping, host mutation, SDN-based) report controller
  overhead as the main cost center and none of them runs a continuous
  simulation in the control plane.
  [MDPI Electronics 2022 survey](https://www.mdpi.com/2079-9292/11/18/2886)
  [arXiv 1905.00964](https://arxiv.org/html/1905.00964v2)
  [Benchmarking SDN controller runtimes for MTD](https://arxiv.org/html/2609.35585v1)
- implication: proteus's neural control plane is the differentiator, and
  its per-tick cost is unique to it; no MTD precedent dictates the idle
  number, the owner's <0.5% is a product requirement.

## decisions taken (r3 phase A)

1. Accumulator event loop with clamp: poll on listener + control peers
   (including peers that connected before they sent), IDLE_POLL_MS=1000
   deadline shared with the token reaper, TICK_CATCHUP_MAX=10 steps per
   wake. idle ~10 ticks/s ~ 0.5% cpu; burst wakes are instant and bring
   empty ticks, so the detector sees the calibrated stimulus density.
   `-d` is wall seconds in live mode.
2. Security timers moved to the wall clock: ban ttl, token ttl, reaper.
   a security timer must not stretch with a slowed simulation clock;
   this was a latent bug of every quiet-time scheme including the old
   --idle-divisor. headless mode keeps sim time and stays bitwise
   replayable.
3. Subscriber/peer tables stay bounded (IPC_CLIENTS_MAX) with loud
   rejection; the oversized-command slot leak and the connect-before-
   send race are closed with regression tests.

## residuals

- sim-time processes (the connectome) run up to 100x slower in long
  quiet; endogenous port moves then also pause. a port move during
  quiet is not defense-relevant (nobody is scanning), but it is a
  published behavior change vs the wall-paced clock.
- wake latency target p99 < 1 ms is client-side measured; the daemon
  side needs the h8 instrument on the bench.
- 8 legitimate short log subscribers can still hold the control table;
  a subscriber cap that reserves a status slot is the closing design
  (blue team residual).
