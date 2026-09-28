#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p results

echo "=== Compiling ==="
gcc -Wall -o counter_bench counter_benchmark.c -lpthread
echo "Built: counter_bench"

echo ""
echo "=== Running counter benchmark ==="
# Single run — table printed to terminal, same data written as CSV
./counter_bench --csv results/counter_results.csv

echo ""
echo "=== Plotting ==="
uv run plot_results.py results/counter_results.csv
echo ""
echo "Done! Check results/ for the CSV and plot."

# Clean up binary
rm -f counter_bench
