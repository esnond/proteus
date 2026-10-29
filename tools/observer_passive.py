#!/usr/bin/env python3
"""
Passive Frequency Analysis Observer — Phase 3 (r3)

Reads party journals (ATLAS output), fits port move distribution,
predicts next port. No active probing, pure statistical modeling.

Outputs: predictions, per-port frequencies, transition matrix.
"""

import sys
import os
import numpy as np
import pandas as pd
from collections import Counter, defaultdict
import json

def read_journal(journal_path):
    """Read party journal v2 CSV, return DataFrame."""
    with open(journal_path, 'r') as f:
        for i, line in enumerate(f):
            if line.startswith('# t_ms'):
                header_line = i
                break
        else:
            raise ValueError(f"No header found in {journal_path}")

    df = pd.read_csv(journal_path, comment='#', header=None, skiprows=header_line)
    if df.shape[1] != 11:
        raise ValueError(f"Expected 11 columns, got {df.shape[1]}")

    df.columns = ['t_ms', 'act_ava', 'act_avb', 'ash_trace', 'ser', 'oct', 'dop',
                  'det_score', 'port', 'cause', 'event']
    return df


class PassiveObserver:
    """
    Passive frequency observer: learns port distribution and transitions
    from observed port sequence only. No probing, no state injection.
    """

    def __init__(self, min_observations=10):
        self.min_observations = min_observations
        self.port_counts = Counter()
        self.transition_counts = defaultdict(Counter)
        self.port_history = []
        self.total_moves = 0

    def observe(self, port):
        """Record observed port move."""
        if self.port_history:
            prev = self.port_history[-1]
            self.transition_counts[prev][port] += 1
        self.port_counts[port] += 1
        self.port_history.append(port)
        self.total_moves += 1

    def predict_next(self, current_port=None):
        """
        Predict next port probability distribution.
        Returns dict {port: probability}.
        """
        if self.total_moves < self.min_observations:
            # Uniform prior before minimum observations
            ports = list(self.port_counts.keys()) or [40000]
            return {p: 1.0 / len(ports) for p in ports}

        if current_port is not None and current_port in self.transition_counts:
            # Use transition model
            trans = self.transition_counts[current_port]
            total = sum(trans.values())
            if total > 0:
                return {p: c / total for p, c in trans.items()}

        # Fallback: marginal distribution
        total = sum(self.port_counts.values())
        if total == 0:
            return {40000: 1.0}
        return {p: c / total for p, c in self.port_counts.items()}

    def predict_next_port(self, current_port=None):
        """Return most likely next port."""
        probs = self.predict_next(current_port)
        return max(probs, key=probs.get)

    def prediction_confidence(self, current_port=None):
        """Return max probability (confidence in prediction)."""
        probs = self.predict_next(current_port)
        return max(probs.values()) if probs else 0.0

    def learnability_L(self, max_obs=500, threshold=0.9):
        """
        Compute L: minimum observations for P(next_port) >= threshold.
        Returns L or max_obs if not reached.
        """
        if len(self.port_history) < self.min_observations + 10:
            return max_obs

        # Simulate online learning: at each prefix, test prediction on next moves
        for obs in range(self.min_observations, min(max_obs, len(self.port_history) - 5), 5):
            # Train on prefix
            prefix_ports = self.port_history[:obs]
            test_ports = self.port_history[obs:obs+10]  # test window

            # Build temporary model from prefix
            temp_obs = PassiveObserver(min_observations=self.min_observations)
            for p in prefix_ports:
                temp_obs.observe(p)

            # Test predictions
            correct = 0
            for i, true_port in enumerate(test_ports):
                if i == 0:
                    current = prefix_ports[-1]
                else:
                    current = test_ports[i-1]
                pred = temp_obs.predict_next_port(current)
                if pred == true_port:
                    correct += 1

            accuracy = correct / len(test_ports) if test_ports else 0
            if accuracy >= threshold:
                return obs

        return max_obs

    def get_model_summary(self):
        """Return summary statistics for reporting."""
        return {
            'total_moves': self.total_moves,
            'unique_ports': len(self.port_counts),
            'port_frequencies': dict(self.port_counts),
            'transition_matrix': {k: dict(v) for k, v in self.transition_counts.items()},
        }


def run_passive_observer(journal_path, max_obs=500):
    """Run passive observer on a party journal, return results dict."""
    df = read_journal(journal_path)

    # Extract port sequence at move points
    port_changes = df['port'].diff() != 0
    move_ports = df.loc[port_changes, 'port'].tolist()

    if len(move_ports) < 2:
        return {'L': max_obs, 'moves': len(move_ports), 'error': 'insufficient_moves'}

    observer = PassiveObserver(min_observations=10)
    for port in move_ports:
        observer.observe(port)

    L = observer.learnability_L(max_obs=max_obs)
    summary = observer.get_model_summary()
    summary['L'] = L
    return summary


def main():
    if len(sys.argv) < 2:
        print("Usage: observer_passive.py <journal.csv> [max_obs]", file=sys.stderr)
        sys.exit(1)

    journal_path = sys.argv[1]
    max_obs = int(sys.argv[2]) if len(sys.argv) > 2 else 500

    result = run_passive_observer(journal_path, max_obs)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
