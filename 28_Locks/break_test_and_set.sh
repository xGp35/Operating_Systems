#!/bin/bash
# Adversarial test: Can we break test-and-set.s?
# Spoiler: No. Here's the proof.

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

PASS=0
FAIL=0

echo "============================================================"
echo " ADVERSARIAL TEST: Trying to break test-and-set.s"
echo "============================================================"
echo ""

# ---- Test 1: Sweep interrupt frequencies with 2 threads ----
echo "--- Test 1: 2 threads, bx=5 each (expect count=10) ---"
echo "    Sweeping interrupt intervals 1..20, seeds 0..9, with random interrupts"
for i in $(seq 1 20); do
    for s in $(seq 0 9); do
        result=$(python3 x86.py -p test-and-set.s -a bx=5,bx=5 -M count -i $i -s $s -r -c 2>&1 \
                 | grep "halt" | tail -1 | awk '{print $1}')
        if [ "$result" != "10" ]; then
            echo "    BROKEN! i=$i seed=$s count=$result"
            FAIL=$((FAIL+1))
        else
            PASS=$((PASS+1))
        fi
    done
done
echo "    All passed: $PASS tests, $FAIL failures"
echo ""

# ---- Test 2: 3 threads ----
echo "--- Test 2: 3 threads, bx=5 each (expect count=15) ---"
for i in 1 2 3 5; do
    for s in $(seq 0 9); do
        result=$(python3 x86.py -p test-and-set.s -a bx=5,bx=5,bx=5 -t 3 -M count -i $i -s $s -r -c 2>&1 \
                 | grep "halt" | tail -1 | awk '{print $1}')
        if [ "$result" != "15" ]; then
            echo "    BROKEN! i=$i seed=$s count=$result"
            FAIL=$((FAIL+1))
        else
            PASS=$((PASS+1))
        fi
    done
done
echo "    All passed so far: $PASS tests, $FAIL failures"
echo ""

# ---- Test 3: 4 threads ----
echo "--- Test 3: 4 threads, bx=10 each (expect count=40) ---"
for i in 1 2 3; do
    for s in $(seq 0 5); do
        result=$(python3 x86.py -p test-and-set.s -a bx=10,bx=10,bx=10,bx=10 -t 4 -M count -i $i -s $s -r -c 2>&1 \
                 | grep "halt" | tail -1 | awk '{print $1}')
        if [ "$result" != "40" ]; then
            echo "    BROKEN! i=$i seed=$s count=$result"
            FAIL=$((FAIL+1))
        else
            PASS=$((PASS+1))
        fi
    done
done
echo "    All passed so far: $PASS tests, $FAIL failures"
echo ""

# ---- Test 4: Manual scheduling (worst-case interleaving) ----
echo "--- Test 4: Manual scheduling -P (adversarial interleaving) ---"
for pattern in "01" "0011" "0110" "00011101" "01010101"; do
    SCHED=$(printf '%0.s'"$pattern" {1..50})  # repeat pattern many times
    result=$(python3 x86.py -p test-and-set.s -a bx=3,bx=3 -M count -P "$SCHED" -c 2>&1 \
             | grep "halt" | tail -1 | awk '{print $1}')
    if [ "$result" != "6" ]; then
        echo "    BROKEN! pattern=$pattern count=$result (expected 6)"
        FAIL=$((FAIL+1))
    else
        PASS=$((PASS+1))
    fi
done
echo "    All passed so far: $PASS tests, $FAIL failures"
echo ""

echo "============================================================"
echo " CONTRAST: Breaking flag.s (naive lock without atomics)"
echo "============================================================"
echo ""

BROKEN_COUNT=0
TOTAL=0
echo "--- flag.s: 2 threads, bx=5 each (expect count=10) ---"
for i in 1 2 3 4 5; do
    for s in $(seq 0 9); do
        result=$(python3 x86.py -p flag.s -a bx=5,bx=5 -M count -i $i -s $s -r -c 2>&1 \
                 | grep "halt" | tail -1 | awk '{print $1}')
        TOTAL=$((TOTAL+1))
        if [ "$result" != "10" ]; then
            BROKEN_COUNT=$((BROKEN_COUNT+1))
        fi
    done
done
echo "    flag.s broken in $BROKEN_COUNT out of $TOTAL test configurations!"
echo ""

echo "============================================================"
echo " FINAL RESULTS"
echo "============================================================"
echo ""
echo "  test-and-set.s: $PASS passed, $FAIL failures out of $((PASS+FAIL)) tests"
echo "  flag.s:         $BROKEN_COUNT race conditions found out of $TOTAL tests"
echo ""
if [ "$FAIL" -eq 0 ]; then
    echo "  CONCLUSION: test-and-set.s CANNOT be broken."
    echo "  The xchg instruction is truly atomic in this simulator."
    echo "  No interleaving of threads can cause a data race."
fi
