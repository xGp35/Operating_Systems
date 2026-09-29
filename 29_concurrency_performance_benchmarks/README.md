# Concurrency Performance Benchmarks

Measures how concurrent data structures scale with increasing thread counts.
Based on OSTEP Chapter 29 homework exercises.

## Quick Start

```bash
./run_benchmarks.sh
```

This compiles, runs, saves CSV results, and generates a plot.

## Manual Usage

```bash
# Compile
gcc -Wall -o counter_bench counter_benchmark.c -lpthread

# Run (human-readable table)
./counter_bench

# Run (CSV for plotting)
./counter_bench --csv > results/counter_results.csv

# Plot (requires uv — https://docs.astral.sh/uv/)
uv run plot_results.py results/counter_results.csv
```

## Adding More Benchmarks

The plotter accepts multiple CSV files, each shown as a separate line with its own marker:

```bash
uv run plot_results.py results/counter_results.csv results/linked_list_results.csv
```

CSV format (with header):
```
threads,time_ms,per_increment_ns
1,48.123,4.81
2,135.456,13.55
```

## Requirements

- C compiler (gcc/clang) + pthreads
- [uv](https://docs.astral.sh/uv/) for plotting (auto-installs matplotlib)
- Works on macOS and Linux
