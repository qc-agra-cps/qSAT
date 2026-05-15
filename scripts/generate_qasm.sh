#!/usr/bin/env bash
# -------------------------------------------------------------------------
# File:    generate_qasm.sh
# Brief:   Convert DIMACS/QDIMACS benchmark files to OpenQASM 2.0 circuits.
#
# Details:
# Reads all benchmark files from test_input/<format>/{fun,add,mul}-bench/
# and writes the resulting QASM circuits to test_output/<format>/{bench}/.
#
# Usage:
#   ./scripts/generate_qasm.sh [dimacs|qdimacs] [fun-bench|add-bench|mul-bench|all]
#
# Example:
#   ./scripts/generate_qasm.sh dimacs
#   ./scripts/generate_qasm.sh qdimacs fun-bench
#   ./scripts/generate_qasm.sh dimacs mul-bench
# -------------------------------------------------------------------------

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ---------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------
FORMAT="${1:-qdimacs}"
BENCH_ARG="${2:-all}"

if [[ "$FORMAT" != "dimacs" && "$FORMAT" != "qdimacs" ]]; then
    echo "[ERROR] Invalid format: '$FORMAT'"
    echo "Usage: $0 [dimacs|qdimacs] [fun-bench|add-bench|mul-bench|all]"
    exit 1
fi

if [[ "$BENCH_ARG" == "all" ]]; then
    BENCHMARKS=("fun-bench" "add-bench" "mul-bench")
elif [[ "$BENCH_ARG" == "fun-bench" || "$BENCH_ARG" == "add-bench" || "$BENCH_ARG" == "mul-bench" ]]; then
    BENCHMARKS=("$BENCH_ARG")
else
    echo "[ERROR] Invalid benchmark: '$BENCH_ARG'"
    echo "Usage: $0 [dimacs|qdimacs] [fun-bench|add-bench|mul-bench|all]"
    exit 1
fi

EXT=".$FORMAT"
BIN_DIR="test_output/$FORMAT"
INPUT_DIR="test_input/$FORMAT"

# ---------------------------------------------------------
# Banner
# ---------------------------------------------------------
echo "=================================================="
echo "         qSAT QASM Generation"
echo "=================================================="
echo " Format    : $FORMAT"
echo " Benchmark : $BENCH_ARG"
echo " Input     : $INPUT_DIR"
echo " Output    : $BIN_DIR"
echo "=================================================="
echo ""

# ---------------------------------------------------------
# Run generation
# ---------------------------------------------------------
TOTAL=0
OK=0
FAIL=0

for BENCH in "${BENCHMARKS[@]}"; do
    SRC="$ROOT_DIR/$INPUT_DIR/$BENCH"
    DST="$ROOT_DIR/$BIN_DIR/$BENCH"

    if [[ ! -d "$SRC" ]]; then
        echo "[SKIP] $BENCH — source directory not found: $SRC"
        continue
    fi

    mkdir -p "$DST"

    echo "--------------------------------------------------"
    echo " Benchmark : $BENCH"
    echo "--------------------------------------------------"

    for INPUT_FILE in "$SRC"/*"$EXT"; do
        [[ -e "$INPUT_FILE" ]] || continue
        STEM="$(basename "$INPUT_FILE" "$EXT")"
        OUTPUT_FILE="$DST/${STEM}.qasm"
        TOTAL=$(( TOTAL + 1 ))

        python3 - <<PYEOF
from pyqsat import dimacs_to_qasm_file
try:
    dimacs_to_qasm_file("$INPUT_FILE", "$OUTPUT_FILE")
    print("  OK  $STEM.qasm")
except Exception as e:
    print(f"  ERR $STEM: {e}")
    raise SystemExit(1)
PYEOF
        OK=$(( OK + 1 ))
    done
    echo ""
done

# ---------------------------------------------------------
# Summary
# ---------------------------------------------------------
echo "=================================================="
echo " Done: $OK / $TOTAL files generated."
echo "=================================================="
