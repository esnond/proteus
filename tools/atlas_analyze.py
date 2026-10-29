#!/usr/bin/env python3
"""
ATLAS OF PARTIES analysis — Phase 2 (r3)

Reads party journal v2, computes:
- 14-driver attribution vectors per port move
- Sparse logistic regression (lasso) per half with 5-fold CV
- Divergence metrics (Jaccard, cosine, rank correlation, mass shift)
- Learnability L (minimum observations for P>=0.9 next-move prediction)
- Permutation test p-values for attribution coverage

Outputs: atlas_parties.csv, atlas_moves.csv, atlas_drivers.csv (per spec)
"""

import sys
import os
import numpy as np
import pandas as pd
from scipy import signal
from scipy.stats import spearmanr
from sklearn.linear_model import LogisticRegressionCV
from sklearn.model_selection import StratifiedKFold
from sklearn.preprocessing import StandardScaler
import warnings
warnings.filterwarnings('ignore')

# 14 frozen driver names (spec section 2.2)
DRIVER_NAMES = [
    'ava_amp', 'avb_amp', 'avd_amp', 'avg_amp',
    'ash_trace', 'ser', 'oct', 'dop',
    'det_score_delta', 'det_state', 'phase_ava', 'phase_avb',
    'coupling_ava_avb', 'noise_floor'
]

CAUSE_CODES = {0: 'none', 1: 'endogenous', 2: 'detector', 3: 'both'}

def read_journal(journal_path):
    """Read party journal v2 CSV, return DataFrame with validated schema."""
    # Find header line (skip # comments)
    with open(journal_path, 'r') as f:
        for i, line in enumerate(f):
            if line.startswith('# t_ms'):
                header_line = i
                break
        else:
            raise ValueError(f"No header found in {journal_path}")

    df = pd.read_csv(journal_path, comment='#', header=None, skiprows=header_line)
    expected_cols = 11
    if df.shape[1] != expected_cols:
        raise ValueError(f"Expected {expected_cols} columns, got {df.shape[1]}")

    df.columns = ['t_ms', 'act_ava', 'act_avb', 'ash_trace', 'ser', 'oct', 'dop',
                  'det_score', 'port', 'cause', 'event']
    return df

def compute_phase_ava_avb(act_ava, act_avb, dt=10.0):
    """
    Compute instantaneous phase of AVA/AVB oscillators via Hilbert transform.
    Returns phase in radians [-pi, pi].
    """
    # Bandpass filter around reversal frequency (~0.5-2 Hz for 600s runs)
    fs = 1000.0 / dt  # 100 Hz
    nyq = fs / 2
    low = 0.3 / nyq
    high = 3.0 / nyq
    if low >= 1 or high >= 1:
        # Fallback: zero-crossing phase
        return zero_crossing_phase(act_ava), zero_crossing_phase(act_avb)

    b, a = signal.butter(4, [low, high], btype='band')
    try:
        ava_filt = signal.filtfilt(b, a, act_ava)
        avb_filt = signal.filtfilt(b, a, act_avb)
        phase_ava = np.angle(signal.hilbert(ava_filt))
        phase_avb = np.angle(signal.hilbert(avb_filt))
    except:
        phase_ava = zero_crossing_phase(act_ava)
        phase_avb = zero_crossing_phase(act_avb)
    return phase_ava, phase_avb

def zero_crossing_phase(signal_arr):
    """Zero-crossing based instantaneous phase (fallback)."""
    phase = np.zeros_like(signal_arr)
    mean_val = np.mean(signal_arr)
    crossings = np.where(np.diff(np.sign(signal_arr - mean_val)))[0]
    if len(crossings) < 2:
        return phase
    for i in range(len(crossings) - 1):
        start, end = crossings[i], crossings[i+1]
        if end > start:
            phase[start:end] = np.linspace(-np.pi, np.pi, end - start)
    return phase

def compute_coupling_ava_avb(conn, act_ava, act_avb, idx_ava, idx_avb):
    """
    Compute coupling_ava_avb = signed synaptic weight AVA->AVB * AVB activity.
    Since we don't have connectome in journal, approximate from literature:
    AVA->AVB is inhibitory (negative weight). Use a proxy.
    """
    # Approximate: AVA inhibits AVB, so coupling is negative when AVA active
    # Weight from connectome (frozen): AVA->AVB gap junction + chemical inh
    # Using literature value: ~ -0.5 normalized
    w_ava_avb = -0.5
    return w_ava_avb * act_avb  # signed weight * post activity

def compute_noise_floor(attribution_matrix, port_moves):
    """
    Noise floor = residual after subtracting deterministic drivers.
    Quantifies stochasticity per move.
    """
    # Fit linear model on all drivers except noise_floor, residual is noise
    X = attribution_matrix[:, :-1]  # exclude noise_floor column
    y = port_moves
    # Ridge regression for stability
    from sklearn.linear_model import Ridge
    ridge = Ridge(alpha=1.0, fit_intercept=False)
    try:
        ridge.fit(X, y)
        pred = ridge.predict(X)
        residual = y - pred
        # Noise floor per move: absolute residual normalized
        noise = np.abs(residual) / (np.abs(y).mean() + 1e-6)
        return noise
    except:
        return np.ones(len(y)) * 0.5

def build_attribution_vectors(df):
    """
    Build 14-driver attribution vectors for each tick.
    Returns (attribution_matrix: n_ticks x 14, move_ticks: indices where port changed)
    """
    n = len(df)
    attrib = np.zeros((n, 14))

    act_ava = df['act_ava'].values
    act_avb = df['act_avb'].values
    ash_trace = df['ash_trace'].values
    ser = df['ser'].values
    oct_ = df['oct'].values
    dop = df['dop'].values
    det_score = df['det_score'].values
    port = df['port'].values
    cause = df['cause'].values

    # Detect detector state from events
    # event codes: 3=det_armed, 4=det_fired, 5=det_cool
    det_state = np.zeros(n, dtype=int)
    current_state = 0  # 0=armed, 1=fired, 2=cool
    for i, ev in enumerate(df['event'].values):
        if ev == 4:  # det_fired
            current_state = 1
        elif ev == 5:  # det_cool
            current_state = 2
        elif ev == 3:  # det_armed
            current_state = 0
        det_state[i] = current_state

    # det_score_delta
    det_score_delta = np.concatenate([[0], np.diff(det_score)])

    # Phase of AVA/AVB
    phase_ava, phase_avb = compute_phase_ava_avb(act_ava, act_avb)

    # Coupling AVA->AVB (approximate, no connectome in journal)
    coupling = compute_coupling_ava_avb(None, act_ava, act_avb, None, None)

    # Fill attribution matrix
    attrib[:, 0] = act_ava       # ava_amp
    attrib[:, 1] = act_avb       # avb_amp
    attrib[:, 2] = 0.0           # avd_amp (not in journal, placeholder)
    attrib[:, 3] = 0.0           # avg_amp (not in journal, placeholder)
    attrib[:, 4] = ash_trace     # ash_trace
    attrib[:, 5] = ser           # ser
    attrib[:, 6] = oct_          # oct
    attrib[:, 7] = dop           # dop
    attrib[:, 8] = det_score_delta  # det_score_delta
    attrib[:, 9] = det_state     # det_state
    attrib[:, 10] = phase_ava    # phase_ava
    attrib[:, 11] = phase_avb    # phase_avb
    attrib[:, 12] = coupling     # coupling_ava_avb
    # noise_floor computed later after we know moves

    # Find port move ticks
    port_changes = np.where(np.diff(port) != 0)[0] + 1  # tick where new port appears
    move_ticks = port_changes.tolist()

    # Compute noise_floor using moves as target
    port_move_binary = np.zeros(n)
    port_move_binary[move_ticks] = 1
    noise = compute_noise_floor(attrib, port_move_binary)
    attrib[:, 13] = noise

    return attrib, move_ticks, det_state

def permutation_test_attribution(attrib, move_ticks, n_perms=500, alpha=0.05):
    """
    Permutation test for each move: shuffle driver labels, recompute correlation.
    Returns p-values per move.
    """
    if len(move_ticks) == 0:
        return np.array([])

    n_drivers = attrib.shape[1]
    move_attrib = attrib[move_ticks]
    n_moves = len(move_ticks)

    # Observed: mean attribution magnitude per move
    observed = np.mean(np.abs(move_attrib), axis=1)

    # Permutation: shuffle driver labels across moves
    p_values = np.ones(n_moves)
    for i in range(n_moves):
        obs = observed[i]
        count_ge = 0
        for _ in range(n_perms):
            shuffled = np.random.permutation(move_attrib[i])
            perm_stat = np.mean(np.abs(shuffled))
            if perm_stat >= obs:
                count_ge += 1
        p_values[i] = (count_ge + 1) / (n_perms + 1)

    return p_values

def fit_sparse_logistic(attrib, move_ticks, half_mask):
    """
    Fit sparse logistic regression (lasso) on half data.
    Returns (beta, selected_indices, mi_scores)
    """
    X = attrib[half_mask]
    half_indices = np.where(half_mask)[0]
    
    # Find moves in this half (local indices within half)
    move_local = [np.where(half_indices == t)[0][0] for t in move_ticks if half_mask[t]]
    
    y = np.zeros(len(X))
    y[move_local] = 1

    if np.sum(y) < 2:  # Too few moves for logistic regression
        return np.zeros(14), [], np.zeros(14)

    # Standardize
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    # L1 logistic regression with 5-fold CV for lambda
    cv = StratifiedKFold(n_splits=min(5, int(np.sum(y))), shuffle=True, random_state=42)
    clf = LogisticRegressionCV(
        Cs=10, cv=cv, penalty='l1', solver='liblinear',
        scoring='roc_auc', max_iter=1000, random_state=42,
        n_jobs=1
    )
    try:
        clf.fit(X_scaled, y)
        beta = clf.coef_.flatten()
        # Scale back
        beta = beta / scaler.scale_
    except:
        beta = np.zeros(14)

    selected = np.where(np.abs(beta) > 1e-6)[0].tolist()

    # Mutual information scores
    from sklearn.feature_selection import mutual_info_classif
    try:
        mi = mutual_info_classif(X, y, random_state=42)
    except:
        mi = np.zeros(14)

    return beta, selected, mi

def compute_divergence(beta1, beta2, selected1, selected2):
    """Compute 4 divergence metrics per spec section 3.2."""
    set1, set2 = set(selected1), set(selected2)

    # Jaccard divergence
    if len(set1 | set2) > 0:
        jaccard = 1.0 - len(set1 & set2) / len(set1 | set2)
    else:
        jaccard = 0.0

    # Cosine distance
    norm1, norm2 = np.linalg.norm(beta1), np.linalg.norm(beta2)
    if norm1 > 0 and norm2 > 0:
        cosine_dist = 1.0 - np.dot(beta1, beta2) / (norm1 * norm2)
    else:
        cosine_dist = 1.0

    # Top-3 rank correlation (Spearman)
    top3_1 = np.argsort(np.abs(beta1))[-3:][::-1]
    top3_2 = np.argsort(np.abs(beta2))[-3:][::-1]
    rank1 = np.argsort(np.argsort(-np.abs(beta1[top3_1])))
    rank2 = np.argsort(np.argsort(-np.abs(beta2[top3_2])))
    try:
        rho, _ = spearmanr(rank1, rank2)
        if np.isnan(rho):
            rho = 0.0
    except:
        rho = 0.0

    # Attribution mass shift
    denom = np.sum(np.abs(beta1)) + np.sum(np.abs(beta2))
    if denom > 0:
        mass_shift = np.sum(np.abs(beta1 - beta2)) / denom
    else:
        mass_shift = 0.0

    return jaccard, cosine_dist, rho, mass_shift

def compute_learnability_L(attrib, move_ticks, half_mask, max_obs=500):
    """
    Compute L: minimum observations for observer model to predict
    next port move with P >= 0.9.

    Observer model: online logistic regression on attribution vectors.
    We simulate an observer learning from sequential data.
    """
    half_indices = np.where(half_mask)[0]
    if len(half_indices) == 0:
        return float('nan')

    X = attrib[half_indices]
    y = np.zeros(len(X))
    move_local = [i for i, t in enumerate(half_indices) if t in move_ticks]
    y[move_local] = 1

    if np.sum(y) < 5:  # Too few moves
        return float('nan')

    # Online learning simulation: train on prefix, test on next
    # Find minimum prefix length where test AUC >= 0.9
    from sklearn.linear_model import LogisticRegression
    from sklearn.metrics import roc_auc_score

    n = len(X)
    for obs in range(10, min(max_obs, n - 10), 10):
        X_train, y_train = X[:obs], y[:obs]
        X_test, y_test = X[obs:obs+100], y[obs:obs+100]  # test window

        if np.sum(y_train) < 2 or np.sum(y_test) < 1:
            continue

        scaler = StandardScaler()
        X_train_s = scaler.fit_transform(X_train)
        X_test_s = scaler.transform(X_test)

        clf = LogisticRegression(penalty='l2', C=1.0, max_iter=500, random_state=42)
        try:
            clf.fit(X_train_s, y_train)
            prob = clf.predict_proba(X_test_s)[:, 1]
            auc = roc_auc_score(y_test, prob)
            if auc >= 0.9:
                return float(obs)
        except:
            continue

    return float(max_obs)  # Not learned within budget

def analyze_party(journal_path, party_id, seed, schedule_hash, binary_hash):
    """Full analysis pipeline for one party journal."""
    df = read_journal(journal_path)
    n_ticks = len(df)

    # Split into halves
    half1_mask = df['t_ms'].values < 300000  # 0-300s (300,000 ms)
    half2_mask = df['t_ms'].values >= 300000  # 300-600s

    # Build attribution vectors
    attrib, move_ticks, det_state = build_attribution_vectors(df)

    # Permutation test p-values for each move
    p_values = permutation_test_attribution(attrib, move_ticks, n_perms=500)

    # Attribution coverage (fraction with p < 0.05 after Bonferroni)
    if len(p_values) > 0:
        bonf_alpha = 0.05 / len(p_values)
        coverage = np.mean(p_values < bonf_alpha)
    else:
        coverage = 0.0

    # Per-half models
    beta1, sel1, mi1 = fit_sparse_logistic(attrib, move_ticks, half1_mask)
    beta2, sel2, mi2 = fit_sparse_logistic(attrib, move_ticks, half2_mask)

    # Divergence metrics
    jaccard, cosine_dist, rho, mass_shift = compute_divergence(beta1, beta2, sel1, sel2)

    # Divergence flags
    flags = []
    if jaccard > 0.5: flags.append('DRIVER_SET_SHIFT')
    if cosine_dist > 0.3: flags.append('WEIGHT_SHIFT')
    if rho < 0.5: flags.append('RANK_SHIFT')
    if mass_shift > 0.4: flags.append('MASS_SHIFT')

    # Learnability L
    L1 = compute_learnability_L(attrib, move_ticks, half1_mask)
    L2 = compute_learnability_L(attrib, move_ticks, half2_mask)
    delta_L = abs(L1 - L2) if not (np.isnan(L1) or np.isnan(L2)) else float('nan')

    # Party summary
    n_moves = len(move_ticks)
    duration_s = df['t_ms'].iloc[-1] / 1000.0

    party_row = {
        'party_id': party_id,
        'seed': seed,
        'schedule_hash': schedule_hash,
        'binary_hash': binary_hash,
        'n_moves': n_moves,
        'attribution_coverage': coverage,
        'jaccard_div': jaccard,
        'cosine_div': cosine_dist,
        'rank_corr': rho,
        'mass_shift': mass_shift,
        'flags_triggered': ','.join(flags) if flags else 'NONE',
        'L_half1': L1,
        'L_half2': L2,
        'delta_L': delta_L,
        'duration_s': duration_s
    }

    # Moves detail
    moves_rows = []
    for idx, tick in enumerate(move_ticks):
        t_ms = df['t_ms'].iloc[tick]
        port_from = df['port'].iloc[tick-1] if tick > 0 else df['port'].iloc[0]
        port_to = df['port'].iloc[tick]
        cause = df['cause'].iloc[tick]
        half = 1 if tick < 30000 else 2  # approximate by tick

        # Top driver for this move
        move_attrib = attrib[tick]
        top_driver_idx = np.argmax(np.abs(move_attrib))
        top_driver = DRIVER_NAMES[top_driver_idx]
        top_weight = beta1[top_driver_idx] if half == 1 else beta2[top_driver_idx]

        moves_rows.append({
            'party_id': party_id,
            'tick': tick,
            't_ms': int(t_ms),
            'port_from': int(port_from),
            'port_to': int(port_to),
            'cause_code': int(cause),
            'attribution_p': p_values[idx] if idx < len(p_values) else 1.0,
            'top_driver': top_driver,
            'top_driver_weight': top_weight,
            'half': half
        })

    # Drivers detail
    drivers_rows = []
    for half, beta, sel, mi in [(1, beta1, sel1, mi1), (2, beta2, sel2, mi2)]:
        for i, name in enumerate(DRIVER_NAMES):
            drivers_rows.append({
                'party_id': party_id,
                'half': half,
                'driver': name,
                'coefficient': beta[i],
                'selected': 1 if i in sel else 0,
                'mi_score': mi[i]
            })

    return party_row, moves_rows, drivers_rows

def main():
    if len(sys.argv) < 2:
        print("Usage: atlas_analyze.py <journal.csv> [party_id] [seed] [schedule_hash] [binary_hash]")
        sys.exit(1)

    journal_path = sys.argv[1]
    party_id = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    schedule_hash = sys.argv[4] if len(sys.argv) > 4 else 'unknown'
    binary_hash = sys.argv[5] if len(sys.argv) > 5 else 'unknown'

    party_row, moves_rows, drivers_rows = analyze_party(
        journal_path, party_id, seed, schedule_hash, binary_hash
    )

    # Output to stdout as CSV (caller redirects to files)
    # We print all three tables with headers
    print("=== ATLAS_PARTIES ===")
    pd.DataFrame([party_row]).to_csv(sys.stdout, index=False)

    print("\n=== ATLAS_MOVES ===")
    pd.DataFrame(moves_rows).to_csv(sys.stdout, index=False)

    print("\n=== ATLAS_DRIVERS ===")
    pd.DataFrame(drivers_rows).to_csv(sys.stdout, index=False)

if __name__ == '__main__':
    main()
