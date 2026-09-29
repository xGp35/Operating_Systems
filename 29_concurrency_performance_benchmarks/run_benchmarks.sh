#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p results

echo "=== Compiling ==="
gcc -Wall -O0 -o counter_bench counter_benchmark.c -lpthread
gcc -Wall -O0 -o approx_bench approx_counter_benchmark.c -lpthread
echo "Built: counter_bench, approx_bench"

echo ""
echo "=== Running simple counter benchmark ==="
./counter_bench --csv results/counter_results.csv

echo ""
echo "=== Running approximate counter benchmark ==="
./approx_bench --csv results

echo ""
echo "=== Plotting ==="
uv run plot_results.py results/counter_results.csv
uv run plot_approx_results.py
echo ""
echo "Done! Check results/ for CSVs and plots."

# Clean up binaries
rm -f counter_bench approx_bench
