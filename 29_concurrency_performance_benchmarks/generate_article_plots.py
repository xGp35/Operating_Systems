#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.12"
# dependencies = ["matplotlib"]
# ///

"""Generates all plots for the Medium article."""

import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import csv

RESULTS_DIR = "results"
os.makedirs(RESULTS_DIR, exist_ok=True)


def read_csv(filepath):
    threads, time_ms, per_inc_ns = [], [], []
    with open(filepath) as f:
        for row in csv.DictReader(f):
            threads.append(int(row["threads"]))
            time_ms.append(float(row["time_ms"]))
            per_inc_ns.append(float(row["per_increment_ns"]))
    return threads, time_ms, per_inc_ns


# ========== Plot 1: Mac M4 Pro results ==========
def plot_mac():
    threads, time_ms, _ = read_csv(f"{RESULTS_DIR}/counter_results.csv")

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(threads, time_ms, marker="o", color="#2196F3", linewidth=2.5, markersize=9)

    # Annotate the key transitions
    ax.annotate("No contention\n~45 ms", xy=(1, time_ms[0]),
                xytext=(2.5, time_ms[0] - 15), fontsize=9,
                arrowprops=dict(arrowstyle="->", color="gray"), color="gray")

    ax.set_xlabel("Number of Threads", fontsize=13)
    ax.set_ylabel("Total Time (ms)", fontsize=13)
    ax.set_title("Concurrent Counter: Mac M4 Pro (12 CPUs)\n10M total increments, constant work",
                 fontsize=14, fontweight="bold")
    ax.set_xscale("log", base=2)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    fig.tight_layout()
    path = f"{RESULTS_DIR}/article_mac_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {path}")


# ========== Plot 2: Linux EC2 results ==========
def plot_linux():
    threads, time_ms, _ = read_csv(f"{RESULTS_DIR}/linux_counter_results.csv")

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(threads, time_ms, marker="s", color="#F44336", linewidth=2.5, markersize=9)

    ax.set_xlabel("Number of Threads", fontsize=13)
    ax.set_ylabel("Total Time (ms)", fontsize=13)
    ax.set_title("Concurrent Counter: Linux EC2 (32 CPUs, AMD EPYC 7R13)\n10M total increments, constant work",
                 fontsize=14, fontweight="bold")
    ax.set_xscale("log", base=2)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    fig.tight_layout()
    path = f"{RESULTS_DIR}/article_linux_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {path}")


# ========== Plot 3: Mac vs Linux comparison ==========
def plot_comparison():
    mac_t, mac_ms, _ = read_csv(f"{RESULTS_DIR}/counter_results.csv")
    linux_t, linux_ms, _ = read_csv(f"{RESULTS_DIR}/linux_counter_results.csv")

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(mac_t, mac_ms, marker="o", color="#2196F3", linewidth=2.5, markersize=9,
            label="Mac M4 Pro (12 CPUs)")
    ax.plot(linux_t, linux_ms, marker="s", color="#F44336", linewidth=2.5, markersize=9,
            label="Linux EC2 (32 CPUs, AMD EPYC)")

    ax.set_xlabel("Number of Threads", fontsize=13)
    ax.set_ylabel("Total Time (ms)", fontsize=13)
    ax.set_title("Concurrent Counter: Mac vs Linux\n10M total increments, constant work",
                 fontsize=14, fontweight="bold")
    ax.set_xscale("log", base=2)
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    ax.set_ylim(bottom=0)

    fig.tight_layout()
    path = f"{RESULTS_DIR}/article_comparison_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {path}")


# ========== Plot 4: NUMA / Thread placement experiment ==========
def plot_numa():
    # Data from our pinned-thread experiments (averaged across runs)
    # 2-thread placement
    labels_2t = [
        "Same core\n(hyperthreads)",
        "Different cores\nsame NUMA",
        "Different\nNUMA nodes",
    ]
    # Average of run 1 and run 4 (the clean runs)
    times_2t = [
        (893 + 996) / 2,    # HT: ~945
        (171 + 169) / 2,    # same NUMA: ~170
        (242 + 231) / 2,    # cross NUMA: ~236
    ]

    # 8 vs 16 thread placement
    labels_8_16 = [
        "8 threads\n1 NUMA node",
        "8 threads\nsplit 4+4 NUMA",
        "16 threads\n8+8 NUMA",
    ]
    times_8_16 = [
        (220 + 220) / 2,    # 8 on node0: ~220
        (236 + 245) / 2,    # 8 split: ~240
        (159 + 160) / 2,    # 16 8+8: ~159
    ]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    colors_2t = ["#E53935", "#43A047", "#1E88E5"]
    bars1 = ax1.bar(labels_2t, times_2t, color=colors_2t, width=0.6, edgecolor="white", linewidth=1.5)
    ax1.set_ylabel("Total Time (ms)", fontsize=12)
    ax1.set_title("2 Threads: Where You Put Them Matters", fontsize=13, fontweight="bold")
    ax1.set_ylim(0, 1100)
    ax1.grid(True, alpha=0.2, axis="y")
    for bar, val in zip(bars1, times_2t):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 20,
                 f"{val:.0f} ms", ha="center", fontsize=11, fontweight="bold")

    colors_8_16 = ["#1E88E5", "#FF8F00", "#43A047"]
    bars2 = ax2.bar(labels_8_16, times_8_16, color=colors_8_16, width=0.6, edgecolor="white", linewidth=1.5)
    ax2.set_ylabel("Total Time (ms)", fontsize=12)
    ax2.set_title("The Surprise: 16 Threads Beats 8", fontsize=13, fontweight="bold")
    ax2.set_ylim(0, 300)
    ax2.grid(True, alpha=0.2, axis="y")
    for bar, val in zip(bars2, times_8_16):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 5,
                 f"{val:.0f} ms", ha="center", fontsize=11, fontweight="bold")

    fig.suptitle("Thread Placement Experiments (Linux EC2, AMD EPYC, pinned threads)",
                 fontsize=14, fontweight="bold", y=1.02)
    fig.tight_layout()
    path = f"{RESULTS_DIR}/article_numa_plot.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {path}")


if __name__ == "__main__":
    plot_mac()
    plot_linux()
    plot_comparison()
    plot_numa()
    print("\nAll article plots generated!")
