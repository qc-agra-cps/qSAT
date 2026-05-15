#!/usr/bin/env bash
# -------------------------------------------------------------------------
# File:    decompose_qasm.sh
# Brief:   Decompose OpenQASM circuits to Clifford+T using qcore.
#
# Details:
# Reads all non-decomposed QASM files from test_output/<format>/{bench}/
# and writes Clifford+T decomposed circuits alongside them, prefixed
# with "decomposed_". Files are processed one at a time, each output
# is fully written before moving to the next.
#
# Usage:
#   ./scripts/decompose_qasm.sh [dimacs|qdimacs] [fun-bench|add-bench|mul-bench|all]
#
# Example:
#   ./scripts/decompose_qasm.sh dimacs fun-bench
#   ./scripts/decompose_qasm.sh qdimacs mul-bench
#   ./scripts/decompose_qasm.sh dimacs
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

BIN_DIR="test_output/$FORMAT"

# ---------------------------------------------------------
# Banner
# ---------------------------------------------------------
echo "=================================================="
echo "         qSAT Clifford+T Decomposition"
echo "=================================================="
echo " Format    : $FORMAT"
echo " Benchmark : $BENCH_ARG"
echo " Directory : $BIN_DIR"
echo "=================================================="
echo ""

# ---------------------------------------------------------
# Run decomposition
# ---------------------------------------------------------
TOTAL=0
OK=0

for BENCH in "${BENCHMARKS[@]}"; do
    DIR="$ROOT_DIR/$BIN_DIR/$BENCH"

    if [[ ! -d "$DIR" ]]; then
        echo "[SKIP] $BENCH — directory not found: $DIR"
        continue
    fi

    echo "--------------------------------------------------"
    echo " Benchmark : $BENCH"
    echo "--------------------------------------------------"

    for INPUT_FILE in "$DIR"/*.qasm; do
        [[ -e "$INPUT_FILE" ]] || continue
        STEM="$(basename "$INPUT_FILE" .qasm)"

        # Skip already-decomposed files
        [[ "$STEM" == decomposed_* ]] && continue

        OUTPUT_FILE="$DIR/decomposed_${STEM}.qasm"
        TOTAL=$(( TOTAL + 1 ))

        python3 - <<PYEOF
from pyqsat import decompose_qasm_file
try:
    decompose_qasm_file("$INPUT_FILE", "$OUTPUT_FILE")
    print("  OK  decomposed_$STEM.qasm")
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
echo " Done: $OK / $TOTAL files decomposed."
echo "=================================================="
