# behaviour comparison: logged move timing against published c. elegans timing

scope: three research_v2 move logs, 600 s simulated parties, one seed (1001) per destination mode. this file compares inter-move intervals to published inter-reversal and dwell statistics. body speed is not logged by our simulator, so no speed comparison appears here.

## 1. published values

all values below were read from the cited full text or abstract on 2026-10-29. a value marked UNCERTAIN was not found in any source i opened and must not be used as a constraint.

| quantity | published value | condition | source |
|---|---|---|---|
| spontaneous reversal rate | 1.29 ± 0.86 per min (mean ± sd, n = 33) | n2 bristol, unseeded NGM agar, 20 C, 30 min crawl then 7 min scoring | [nagele, 2004] |
| basal pirouette rate | 0.0197 events/s = 1.18 per min | n2, agar chemotaxis plate, dC/dT = 0 | [iino, 2009] |
| pirouette initiation rate | 0.0234 ± 0.00205 to 0.0296 ± 0.00213 per s = 1.40 to 1.78 per min | n2, agar, biotin, NH4Cl and uniform attractant | [pierce-shimomura, 1999] |
| sharp turns | about twice per minute | n2, moist agar | [pierce-shimomura, 1999] |
| mean forward dwell, time between reversals | 16.3 ± 0.3 s, exponential, implies 3.68 per min | n2, agar, off food, 33 worms x 35 min | [stephens, 2011] |
| run duration, short and long modes | tau_s = 2.38 s, tau_l = 26.1 s, t_crit = 6.05 s | n2, agar, chemotaxis | [pierce-shimomura, 1999] |
| pirouette gap definition | sharp turns separated by less than 3.18 s | n2, agar | [iino, 2009] |
| forward attractor oscillation | tau_osc = 1.90 ± 0.15 s | n2, agar, off food | [stephens, 2011] |
| posture dimensionality | 4 modes, about 95% of body shape variance | n2, agar | [stephens, 2011] |
| posture dimensionality | 5 eigenworm coefficients | n2, agar, off food | [costa, 2024] |
| dwell distribution shape | exponential decay of forward survival | n2, agar, off food | [stephens, 2011] |
| dwell distribution shape | heavy-tailed first passage time distributions and correlation functions | n2, agar, off food, 12 worms, 35 min, 16 Hz | [costa, 2024] |
| coefficient of variation of forward dwell | not reported; the quoted exponential decay implies CV = 1 (derived here, not published) | n2, agar, off food | [stephens, 2011] |
| reversal or pirouette rate in liquid | UNCERTAIN | liquid | none found |
| omega turn duration | UNCERTAIN | agar | none found |

notes on the table:

- [nagele, 2004] measured reversals directly, with the tail of the nose moving backwards, in air on an unseeded plate. the same table reports 3.5 ± 3.2% of time moving backwards in air.
- [iino, 2009] reports 0.0197 events/s as the real-animal pirouette frequency at dC/dT = 0; it is used there as the basal parameter of a model and is stated to be the measured frequency.
- [pierce-shimomura, 1999] reports "interrupted approximately twice a minute by a sharp turn" and separate pirouette initiation rates of 0.0234, 0.0264 and 0.0296 per s for biotin, NH4Cl and uniform conditions.
- [stephens, 2011] calls 16.3 s the "mean interval" and the survival time in the forward state; i treat it as the mean inter-reversal interval because the paper defines reversal as the phase-velocity sign change that ends that interval. the implied rate, 60/16.3 = 3.68 per min, is arithmetic, not a quoted number.
- [costa, 2024] reports heavy tails qualitatively in the abstract and figure captions. the numeric power-law fit t^-1.97 appearing in its fig. 1b is for rats, not worms, and is therefore not used here. no measured worm tail exponent was found.
- [stephens, 2011] and [costa, 2024] disagree on the tail: an exponential forward dwell in the first, heavy-tailed first passage times in the second. the difference is consistent with the later paper adding slow behavioural adaptation and longer observation, but both cannot describe the same stationary process.

## 2. our numbers

source files: `/home/esnond/proteus/results/research_v2/{bio,cycle,random}_1001_moves.csv`. each row is a realised move, `t_ms,port`. the engine-native reversal rule is signed body speed below -0.02 mm/s sustained 500 ms, as recorded in `/home/esnond/proteus/README.md`. inter-move interval is used as a proxy for inter-reversal interval, not as a measured reversal.

command run:

    python3 - <<'EOF'
    import csv, glob, statistics as st, os
    for f in sorted(glob.glob('/home/esnond/proteus/results/research_v2/*_moves.csv')):
        ts=[]
        with open(f) as fh:
            r=csv.DictReader(fh)
            for row in r:
                ts.append(float(row['t_ms']))
        d=[(ts[i+1]-ts[i])/1000.0 for i in range(len(ts)-1)]
        n=len(ts)
        mean=sum(d)/len(d)
        sd=st.stdev(d)
        print(os.path.basename(f))
        print('  moves =', n)
        print('  first/last t_ms =', ts[0], ts[-1])
        print('  intervals n =', len(d))
        print('  mean interval s = %.3f' % mean)
        print('  median interval s = %.3f' % st.median(d))
        print('  CV (sample sd/mean) = %.3f' % (sd/mean))
        print('  CV (population sd/mean) = %.3f' % (st.pstdev(d)/mean))
        print('  max interval s = %.3f' % max(d))
        print('  min interval s = %.3f' % min(d))
        print('  span-based rate moves/min = %.3f' % (n/((ts[-1]-ts[0])/60000.0)))
        print('  fixed 600s rate moves/min = %.3f' % (n/10.0))
        print('  interval-based rate/min = %.3f' % (60.0/mean))
    EOF

result:

| mode | moves | first/last t (s) | mean interval (s) | median interval (s) | CV (sample sd/mean) | max interval (s) | min interval (s) | rate, moves per 600 s |
|---|---|---|---|---|---|---|---|---|
| bio | 69 | 0.510 / 588.490 | 8.647 | 3.830 | 1.189 | 48.700 | 2.000 | 6.90 |
| cycle | 81 | 1.000 / 588.490 | 7.344 | 2.800 | 1.302 | 45.520 | 2.000 | 8.10 |
| random | 80 | 0.510 / 588.490 | 7.443 | 2.800 | 1.305 | 45.520 | 0.490 | 8.00 |

the interval-based rate, 60/mean, is 6.94, 8.17 and 8.06 per min for bio, cycle and random. the span-based rate, moves divided by the observed first-to-last span, is 7.04, 8.27 and 8.16 per min. all three definitions are within 0.2 per min of the table value. the population-sd CV is 1.180, 1.294 and 1.297.

## 3. which comparisons are legitimate

legitimate:

- rate of reversal-like events per minute is the only comparison with a matching unit. it is a rate over a fixed simulated interval, and the published rates are also events per unit real time.
- interval distribution shape can be compared qualitatively: ours has CV 1.19 to 1.31, close to the CV of 1 implied by an exponential. this is consistent with [stephens, 2011] and inconsistent with the heavy-tailed regime of [costa, 2024], but n = 68 to 80 intervals is far too small to test a tail.
- the ordering across modes is an internal result: cycle and random fire about 1.16x more often than bio, and their means and medians are nearly identical. this is not a biological claim.

not legitimate:

- our clock is model time, the animal data are real time. a 600 s simulation is not a 600 s recording of a worm, and nothing here establishes that the engine's time step maps to seconds of animal behaviour. the file names use t_ms, which is model milliseconds, and the comparison assumes that one model second is one animal second. that assumption is untested.
- our event is a threshold crossing of simulated signed body speed, not a visually or kinematically scored reversal. the published assays score a nose that moves backwards or a phase-velocity sign change. the two definitions can differ by missing short reversals, by splitting one reversal into several threshold crossings, and by counting a sustained backward phase that an observer would call one event.
- a port change may not equal a reversal. one port change per reversal is the intended mapping, not a verified one. moves that are turns or escapes not caused by the reversal rule would inflate the rate.
- n = 1 seed per destination mode, and the same seed 1001 is shared by all three. no confidence interval, no seed range, no held-out seed. every number in the table is a single realisation.
- the published conditions differ from each other: unseeded agar, attractant gradients, and off-food plates; 19 to 24 C; group sizes from 7 to 47 animals. our simulator has no agar, no food patch and no temperature in these logs. matching a rate across these conditions is at best an order-of-magnitude check.
- median versus mean is not comparable. our median is 2.8 to 3.83 s while the published central value is a mean of 16.3 s [stephens, 2011]. the mean-to-mean comparison is the one used below.
- speed, body length, curvature, omega turns and pirouette geometry are out of scope. body speed is not logged.

plain discrepancy statements, factor of two rule:

- against 1.29 per min [nagele, 2004], our 6.90 to 8.10 per min is 5.4x to 6.3x higher. this exceeds a factor of two.
- against 1.18 per min [iino, 2009], ours is 5.8x to 6.9x higher. this exceeds a factor of two.
- against 1.40 to 1.78 per min [pierce-shimomura, 1999], ours is 3.9x to 5.8x higher. this exceeds a factor of two.
- against 3.68 per min implied by 16.3 s [stephens, 2011], ours is 1.88x (bio), 2.22x (cycle) and 2.19x (random) higher. cycle and random exceed a factor of two; bio is just below it.
- the mean interval is 1.88x to 2.22x shorter than 16.3 s. the median interval, 2.8 to 3.83 s, is 4.3x to 5.8x shorter, but median versus mean is not a valid ratio.
- our CV of 1.19 to 1.31 is within 31% of the exponential value of 1, so it does not support a heavy tail. with 68 to 80 intervals and a maximum of 45.5 to 48.7 s, the data cannot distinguish an exponential from a truncated power law.

## verdict

the largest discrepancy that the data actually support is the reversal-proxy rate: 6.90 to 8.10 moves per min in the simulator against 1.29 ± 0.86 spontaneous reversals per min on unseeded agar [nagele, 2004], a factor of 5.4 to 6.3. against the best-matched off-food agar dwell measurement, 16.3 s mean forward interval [stephens, 2011], the discrepancy is smaller, 1.88x to 2.22x, and for the bio mode it is under a factor of two. the rate gap is therefore real within the stated proxy but its size depends on which published assay is chosen; the nagele comparison is the least matched because it scores short spontaneous reversals on an unseeded plate, while our rule fires on any sustained backward body speed.

## statements i could not source

1. reversal or pirouette frequency in liquid, in reversals per minute: no numeric value found in any paper i opened. UNCERTAIN.
2. omega turn duration in seconds: not found in [pierce-shimomura, 1999], [iino, 2009] or [suwazono, 2025]; the last pools omega and delta turns and reports counts only. UNCERTAIN.
3. a measured power-law exponent for c. elegans dwell times: [costa, 2024] reports heavy tails but the only numeric exponent in the text, t^-1.97, belongs to rat choice data. UNCERTAIN for worms.
4. the temperature of the [stephens, 2011] tracking experiments: not stated in the fetched text.
5. a published coefficient of variation for inter-reversal intervals: none found; CV = 1 is my arithmetic consequence of the quoted exponential.
6. the mapping one port change = one reversal: internal assumption, not sourced.
7. the mapping one model second = one animal second: internal assumption, not sourced.
8. the engine rule constants as behaviourally calibrated values: the rule is documented in `/home/esnond/proteus/README.md` as an engine-native reflex, not as a fit to worm reversal statistics.
9. whether the published rates were collected on the same substrate, food state and temperature as each other: they were not, and no source reconciles them.

## sources

- [nagele, 2004] nagele p, metz lb, crowder cm. nitrous oxide (n2o) requires the n-methyl-d-aspartate receptor for its action in caenorhabditis elegans. proc natl acad sci usa. 2004. https://pmc.ncbi.nlm.nih.gov/articles/PMC423274/
- [iino, 2009] iino y, yoshida k. parallel use of two behavioral mechanisms for chemotaxis in caenorhabditis elegans. j neurosci. 2009. https://pmc.ncbi.nlm.nih.gov/articles/PMC6665864/
- [pierce-shimomura, 1999] pierce-shimomura jt, morse tm, lockery sr. the fundamental role of pirouettes in caenorhabditis elegans chemotaxis. j neurosci. 1999. https://pmc.ncbi.nlm.nih.gov/articles/PMC6782915/
- [stephens, 2011] stephens gj, bueno de mesquita m, ryu ws, bialek w. emergence of long timescales and stereotyped behaviors in caenorhabditis elegans. proc natl acad sci usa. 2011. https://pmc.ncbi.nlm.nih.gov/articles/PMC3088607/
- [costa, 2024] costa ac, sridhar g, wyart c, vergassola m. fluctuating landscapes and heavy tails in animal behavior. prx life. 2024. https://pmc.ncbi.nlm.nih.gov/articles/PMC12393825/
- [suwazono, 2025] suwazono k, kuze k, tazawa ut, jang ms, kunitomo h, toyoshima y, iino y. dissection of behavioral components and the role of omega/delta turns for the chemotaxis of c. elegans. genes cells. 2025. https://pmc.ncbi.nlm.nih.gov/articles/PMC12120540/
