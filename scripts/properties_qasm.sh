#!/usr/bin/env bash
# -------------------------------------------------------------------------
# File:    properties_qasm.sh
# Brief:   Extract circuit properties (T-count, gate counts) from decomposed
#          Clifford+T QASM circuits.
#
# Details:
# Reads all decomposed_*.qasm files from test_output/<format>/{bench}/
# and prints gate summary and T-count for each.
#
# Usage:
#   ./scripts/properties_qasm.sh [dimacs|qdimacs] [fun-bench|add-bench|mul-bench|all]
#
# Example:
#   ./scripts/properties_qasm.sh qdimacs fun-bench
#   ./scripts/properties_qasm.sh dimacs mul-bench
#   ./scripts/properties_qasm.sh qdimacs
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
echo "         qSAT Circuit Properties"
echo "=================================================="
echo " Format    : $FORMAT"
echo " Benchmark : $BENCH_ARG"
echo " Directory : $BIN_DIR"
echo "=================================================="
echo ""

# ---------------------------------------------------------
# Extract properties
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

    for INPUT_FILE in "$DIR"/decomposed_*.qasm; do
        [[ -e "$INPUT_FILE" ]] || continue
        STEM="$(basename "$INPUT_FILE" .qasm)"
        TOTAL=$(( TOTAL + 1 ))

        python3 - <<PYEOF
import json
from pyqsat import qasm_properties
try:
    props = qasm_properties("$INPUT_FILE")
    gate_summary = props.get("GateSummary") or {}
    t = sum(v for k, v in gate_summary.items() if str(k).lower() == "t")
    tdg = sum(v for k, v in gate_summary.items() if str(k).lower() == "tdg")
    props["t-count"] = t + tdg
    print("  $STEM")
    print(json.dumps(props, indent=4, sort_keys=True))
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
echo " Done: $OK / $TOTAL files processed."
echo "=================================================="
