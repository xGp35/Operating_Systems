#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.12"
# dependencies = ["matplotlib"]
# ///

"""
Plots approximate counter benchmark results and compares with simple counter.

Usage:
    uv run plot_approx_results.py

Expects these files in results/:
    counter_results.csv              (simple counter: threads,time_ms,per_increment_ns)
    approx_S1_results.csv            (approx with S=1)
    approx_S10_results.csv           (approx with S=10)
    approx_S64_results.csv           (approx with S=64)
    approx_S512_results.csv          (approx with S=512)
    approx_S1024_results.csv         (approx with S=1024)
    approx_by_threshold_results.csv  (threshold,time_ms,per_increment_ns)
"""

import csv
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RESULTS_DIR = "results"


def read_csv(filepath):
    """Read a CSV file with threads,time_ms,per_increment_ns."""
    threads, time_ms, per_inc_ns = [], [], []
    with open(filepath) as f:
        for row in csv.DictReader(f):
            threads.append(int(row["threads"]))
            time_ms.append(float(row["time_ms"]))
            per_inc_ns.append(float(row["per_increment_ns"]))
    return threads, time_ms, per_inc_ns


def read_threshold_csv(filepath):
    """Read a CSV file with threshold,time_ms,per_increment_ns."""
    thresholds, time_ms, per_inc_ns = [], [], []
    with open(filepath) as f:
        for row in csv.DictReader(f):
            thresholds.append(int(row["threshold"]))
            time_ms.append(float(row["time_ms"]))
            per_inc_ns.append(float(row["per_increment_ns"]))
    return thresholds, time_ms, per_inc_ns


def plot_comparison():
    """Plot A: Simple counter vs approximate counter at various thresholds."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

    # Style definitions
    styles = [
        {"label": "Simple counter",  "marker": "o", "color": "#E53935", "ls": "-",  "lw": 2.5, "ms": 9, "zorder": 10},
        {"label": "Approx S=1",      "marker": "x", "color": "#9E9E9E", "ls": ":",  "lw": 1.5, "ms": 7, "zorder": 5},
        {"label": "Approx S=10",     "marker": "v", "color": "#FF9800", "ls": "--", "lw": 2,   "ms": 7, "zorder": 5},
        {"label": "Approx S=64",     "marker": "^", "color": "#2196F3", "ls": "-.", "lw": 2,   "ms": 7, "zorder": 5},
        {"label": "Approx S=512",    "marker": "s", "color": "#4CAF50", "ls": "--", "lw": 2,   "ms": 7, "zorder": 5},
        {"label": "Approx S=1024",   "marker": "D", "color": "#1B5E20", "ls": "-",  "lw": 2.5, "ms": 8, "zorder": 8},
    ]

    files = [
        f"{RESULTS_DIR}/counter_results.csv",
        f"{RESULTS_DIR}/approx_S1_results.csv",
        f"{RESULTS_DIR}/approx_S10_results.csv",
        f"{RESULTS_DIR}/approx_S64_results.csv",
        f"{RESULTS_DIR}/approx_S512_results.csv",
        f"{RESULTS_DIR}/approx_S1024_results.csv",
    ]

    for filepath, style in zip(files, styles):
        if not os.path.exists(filepath):
            print(f"Warning: {filepath} not found, skipping")
            continue
        threads, time_ms, per_inc_ns = read_csv(filepath)
        ax1.plot(threads, time_ms, label=style["label"],
                 marker=style["marker"], color=style["color"],
                 linestyle=style["ls"], linewidth=style["lw"],
                 markersize=style["ms"], zorder=style["zorder"])
        ax2.plot(threads, per_inc_ns, label=style["label"],
                 marker=style["marker"], color=style["color"],
                 linestyle=style["ls"], linewidth=style["lw"],
                 markersize=style["ms"], zorder=style["zorder"])

    for ax in (ax1, ax2):
        ax.set_xlabel("Number of Threads", fontsize=12)
        ax.legend(fontsize=9)
        ax.grid(True, alpha=0.3)
        ax.set_xscale("log", base=2)
        ax.set_ylim(bottom=0)

    ax1.set_ylabel("Total Time (ms)", fontsize=12)
    ax1.set_title("Wall-Clock Time vs Thread Count", fontsize=13)
    ax2.set_ylabel("Per-Increment Cost (ns)", fontsize=12)
    ax2.set_title("Per-Increment Cost vs Thread Count", fontsize=13)

    fig.suptitle("Simple Counter vs Approximate Counter\n(10M total increments, constant work)",
                 fontsize=14, fontweight="bold", y=1.02)
    fig.tight_layout()

    path = f"{RESULTS_DIR}/approx_comparison_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Plot saved to {path}")


def plot_threshold_sweep():
    """Plot B: Time vs threshold for fixed thread count = num_cpus."""
    filepath = f"{RESULTS_DIR}/approx_by_threshold_results.csv"
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found, skipping threshold sweep plot")
        return

    thresholds, time_ms, per_inc_ns = read_threshold_csv(filepath)

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(thresholds, time_ms, marker="o", color="#2196F3", linewidth=2.5, markersize=9)

    ax.set_xlabel("Threshold (S)", fontsize=13)
    ax.set_ylabel("Total Time (ms)", fontsize=13)
    ax.set_title("Approximate Counter: Effect of Threshold S\n(threads = num_cpus, 10M total increments)",
                 fontsize=14, fontweight="bold")
    ax.set_xscale("log", base=2)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    fig.tight_layout()
    path = f"{RESULTS_DIR}/approx_threshold_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Plot saved to {path}")


if __name__ == "__main__":
    plot_comparison()
    plot_threshold_sweep()
