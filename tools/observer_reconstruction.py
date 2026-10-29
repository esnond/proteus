#!/usr/bin/env python3
"""
Generator Reconstruction Observer — Phase 3 (r3)

Fits xorshift64* seed or schedule parameters from observed ports.
Implements g6: model-collapse caveat - synthetic-trained observer
transfer gap to independently parameterized stand.

This observer attempts to reconstruct the generative process:
- For seeded schedules: recover the seed
- For bio-stochastic: recover schedule parameters
"""

import sys
import numpy as np
import pandas as pd
from collections import Counter
import json
import hashlib


def read_journal(journal_path):
    """Read party journal v2 CSV."""
    with open(journal_path, 'r') as f:
        for i, line in enumerate(f):
            if line.startswith('# t_ms'):
                header_line = i
                break
        else:
            raise ValueError(f"No header found in {journal_path}")

    df = pd.read_csv(journal_path, comment='#', header=None, skiprows=header_line)
    df.columns = ['t_ms', 'act_ava', 'act_avb', 'ash_trace', 'ser', 'oct', 'dop',
                  'det_score', 'port', 'cause', 'event']
    return df


class Xorshift64Star:
    """
    xorshift64* PRNG - exact implementation matching proteus C code.
    Used for seed reconstruction.
    """
    def __init__(self, seed):
        self.state = seed & 0xFFFFFFFFFFFFFFFF

    def next(self):
        x = self.state
        x ^= (x >> 12) & 0xFFFFFFFFFFFFFFFF
        x ^= (x << 25) & 0xFFFFFFFFFFFFFFFF
        x ^= (x >> 27) & 0xFFFFFFFFFFFFFFFF
        self.state = x
        return (x * 0x2545F4914F6CDD1D) & 0xFFFFFFFFFFFFFFFF

    def next_float(self):
        return self.next() / 2**64


def simulate_schedule_from_seed(seed, schedule_file, duration_s=600):
    """
    Simulate port schedule from seed using proteus logic.
    This is a simplified version - in reality would call proteusd or
    reimplement the schedule generation logic.
    For now, return a deterministic port sequence based on seed.
    """
    rng = Xorshift64Star(seed)

    # Parse schedule file to get event structure
    events = []
    with open(schedule_file, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split(',')
            if len(parts) >= 2:
                try:
                    t_ms = int(parts[0])
                    kind = parts[1]
                    events.append((t_ms, kind))
                except:
                    pass

    # Simplified: generate port moves based on events + RNG
    ports = [40000]  # initial port
    port_range = list(range(40000, 40100))  # example range

    for t_ms, kind in events:
        if t_ms > duration_s * 1000:
            break
        if kind == 'burst':
            # Generate burst of moves
            for _ in range(5):
                port = port_range[rng.next() % len(port_range)]
                ports.append(port)

    return ports


class GeneratorReconstructionObserver:
    """
    Reconstructs generator parameters from observed port sequence.

    Two modes:
    1. Seed reconstruction: brute-force search over seed space
    2. Parameter reconstruction: fit schedule parameters (rate, burstiness, etc.)
    """

    def __init__(self, schedule_file, max_seed_search=10000):
        self.schedule_file = schedule_file
        self.max_seed_search = max_seed_search

    def reconstruct_seed(self, observed_ports, max_seeds=None):
        """
        Brute-force seed reconstruction.
        Returns (best_seed, match_score, candidates).
        """
        if max_seeds is None:
            max_seeds = self.max_seed_search

        observed = observed_ports[:50]  # use first 50 moves
        best_seed = None
        best_score = -1
        candidates = []

        # Search seed space
        for seed in range(max_seeds):
            try:
                simulated = simulate_schedule_from_seed(seed, self.schedule_file)
                if len(simulated) < len(observed):
                    continue

                # Score: fraction of matching ports in sequence
                matches = sum(1 for o, s in zip(observed, simulated) if o == s)
                score = matches / len(observed)

                if score > best_score:
                    best_score = score
                    best_seed = seed

                if score > 0.8:  # high confidence candidates
                    candidates.append((seed, score))

            except Exception:
                continue

        return best_seed, best_score, candidates

    def reconstruct_parameters(self, observed_ports):
        """
        Fit schedule parameters: move rate, burst size, port entropy, etc.
        """
        if len(observed_ports) < 2:
            return {}

        # Inter-move intervals (from journal we'd have times; here approximate)
        moves = len(observed_ports)

        # Port diversity (entropy)
        port_counts = Counter(observed_ports)
        total = sum(port_counts.values())
        port_probs = [c / total for c in port_counts.values()]
        port_entropy = -sum(p * np.log2(p) for p in port_probs)

        # Transition entropy
        transitions = Counter(zip(observed_ports[:-1], observed_ports[1:]))
        trans_total = sum(transitions.values())
        trans_probs = [c / trans_total for c in transitions.values()]
        trans_entropy = -sum(p * np.log2(p) for p in trans_probs)

        # Burst detection: consecutive moves within short window
        # (would need timing from journal)

        return {
            'n_moves': moves,
            'unique_ports': len(port_counts),
            'port_entropy': port_entropy,
            'transition_entropy': trans_entropy,
            'port_distribution': dict(port_counts),
        }

    def compute_transfer_gap(self, train_journals, test_journals):
        """
        Compute g6 transfer gap: model trained on synthetic (train)
        evaluated on independent stand (test).
        """
        # Train: learn parameter distribution from train journals
        train_params = []
        for jpath in train_journals:
            df = read_journal(jpath)
            port_changes = df['port'].diff() != 0
            ports = df.loc[port_changes, 'port'].tolist()
            params = self.reconstruct_parameters(ports)
            train_params.append(params)

        # Average parameters
        avg_params = {}
        for key in train_params[0].keys():
            if isinstance(train_params[0][key], (int, float)):
                avg_params[key] = np.mean([p[key] for p in train_params])

        # Test: predict on test journals using learned parameters
        test_scores = []
        for jpath in test_journals:
            df = read_journal(jpath)
            port_changes = df['port'].diff() != 0
            ports = df.loc[port_changes, 'port'].tolist()
            test_params = self.reconstruct_parameters(ports)

            # Score: parameter distance
            score = 0
            for key in avg_params:
                if key in test_params and isinstance(test_params[key], (int, float)):
                    score += abs(test_params[key] - avg_params[key]) / (abs(avg_params[key]) + 1e-6)
            test_scores.append(score / len(avg_params) if avg_params else 0)

        return {
            'train_params': avg_params,
            'test_scores': test_scores,
            'mean_transfer_gap': np.mean(test_scores),
            'transfer_gap_ci95': 1.96 * np.std(test_scores) / np.sqrt(len(test_scores)) if len(test_scores) > 1 else 0,
        }


def run_reconstruction_observer(journal_path, schedule_file, max_seed_search=10000):
    """Run reconstruction observer on single journal."""
    df = read_journal(journal_path)
    port_changes = df['port'].diff() != 0
    observed_ports = df.loc[port_changes, 'port'].tolist()

    observer = GeneratorReconstructionObserver(schedule_file, max_seed_search)

    # Seed reconstruction
    best_seed, match_score, candidates = observer.reconstruct_seed(observed_ports)

    # Parameter reconstruction
    params = observer.reconstruct_parameters(observed_ports)

    return {
        'reconstructed_seed': best_seed,
        'seed_match_score': match_score,
        'seed_candidates': candidates[:10],  # top 10
        'schedule_parameters': params,
        'n_observed_moves': len(observed_ports),
    }


def main():
    if len(sys.argv) < 3:
        print("Usage: observer_reconstruction.py <journal.csv> <schedule.csv> [max_seed_search]", file=sys.stderr)
        sys.exit(1)

    journal_path = sys.argv[1]
    schedule_file = sys.argv[2]
    max_seed_search = int(sys.argv[3]) if len(sys.argv) > 3 else 10000

    result = run_reconstruction_observer(journal_path, schedule_file, max_seed_search)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
