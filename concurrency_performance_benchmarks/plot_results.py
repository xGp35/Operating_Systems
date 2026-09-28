#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.12"
# dependencies = ["matplotlib"]
# ///

"""
Plots concurrency benchmark results from CSV files.

Usage:
    uv run plot_results.py results/counter_results.csv
    uv run plot_results.py results/counter.csv results/linked_list.csv results/hash_table.csv

Each CSV file becomes a separate line on the plot with its own marker.
CSV format: threads,time_ms,per_increment_ns (with header row)
"""

import csv
import sys
import os
import matplotlib
matplotlib.use("Agg")  # non-interactive backend — saves to file without blocking
import matplotlib.pyplot as plt

# Markers and colors for different data structures
STYLES = [
    {"marker": "o", "color": "#2196F3", "linestyle": "-"},   # blue circle
    {"marker": "s", "color": "#F44336", "linestyle": "--"},   # red square
    {"marker": "^", "color": "#4CAF50", "linestyle": "-."},   # green triangle
    {"marker": "D", "color": "#FF9800", "linestyle": ":"},    # orange diamond
    {"marker": "v", "color": "#9C27B0", "linestyle": "-"},    # purple inverted triangle
]


def read_csv(filepath):
    """Read a benchmark CSV file. Returns (threads[], time_ms[], per_inc_ns[])."""
    threads = []
    time_ms = []
    per_inc_ns = []
    with open(filepath, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            threads.append(int(row["threads"]))
            time_ms.append(float(row["time_ms"]))
            per_inc_ns.append(float(row["per_increment_ns"]))
    return threads, time_ms, per_inc_ns


def label_from_filename(filepath):
    """Derive a human-readable label from the CSV filename.
    'results/counter_results.csv' -> 'counter'
    """
    name = os.path.splitext(os.path.basename(filepath))[0]
    # Remove common suffixes
    for suffix in ["_results", "_benchmark", "_bench"]:
        name = name.replace(suffix, "")
    return name.replace("_", " ").title()


def plot(csv_files, output_path="results/benchmark_plot.png"):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    for i, filepath in enumerate(csv_files):
        style = STYLES[i % len(STYLES)]
        label = label_from_filename(filepath)
        threads, time_ms, per_inc_ns = read_csv(filepath)

        ax1.plot(threads, time_ms, label=label,
                 marker=style["marker"], color=style["color"],
                 linestyle=style["linestyle"], linewidth=2, markersize=8)

        ax2.plot(threads, per_inc_ns, label=label,
                 marker=style["marker"], color=style["color"],
                 linestyle=style["linestyle"], linewidth=2, markersize=8)

    # Left plot: total time
    ax1.set_xlabel("Number of Threads", fontsize=12)
    ax1.set_ylabel("Total Time (ms)", fontsize=12)
    ax1.set_title("Wall-Clock Time vs Thread Count", fontsize=13)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale("log", base=2)

    # Right plot: per-increment cost
    ax2.set_xlabel("Number of Threads", fontsize=12)
    ax2.set_ylabel("Per-Increment Cost (ns)", fontsize=12)
    ax2.set_title("Lock Contention Cost vs Thread Count", fontsize=13)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale("log", base=2)

    fig.suptitle("Concurrency Performance Benchmarks\n(10M total increments, constant work)",
                 fontsize=14, fontweight="bold", y=1.02)
    fig.tight_layout()

    os.makedirs(os.path.dirname(output_path) if os.path.dirname(output_path) else ".", exist_ok=True)
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    print(f"Plot saved to {output_path}")

    plt.close(fig)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: uv run plot_results.py <csv_file> [csv_file2] ...")
        print("Example: uv run plot_results.py results/counter_results.csv")
        sys.exit(1)

    csv_files = sys.argv[1:]
    for f in csv_files:
        if not os.path.exists(f):
            print(f"Error: file not found: {f}")
            sys.exit(1)

    plot(csv_files)
