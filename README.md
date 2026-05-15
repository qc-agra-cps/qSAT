# qSAT

qSAT is a tool for synthesizing Grover oracle circuits from Boolean satisfiability (SAT) formulas.
It supports both conventional CNF and ESOP-based e-CNF encodings, enabling direct comparison of
quantum resource requirements (qubit count, T-gate complexity, circuit depth) between the two representations.

The framework supports:
- DIMACS CNF format (`.dimacs`): classical Tseitin encoding
- QDIMACS ECNF format (`.qdimacs`): ESOP-based e-CNF encoding

---

# Research Reference

If you use qSAT in academic work, please cite:

```bibtex
@article{?,
  title={Performance Gains in Quantum SAT Solvers Using ESOP Encoding},
  author={Assaad, Majd and Kole, Abhoy and Drechsler, Rolf},
  journal={?},
  year={2025}
}
```

---

# Features

- Quantum oracle (OpenQASM 2.0) synthesis from CNF and e-CNF SAT instances
- Clifford+T decomposition via qcore submodule for Grover oracle circuits
- Side-by-side resource comparison: qubit count, T-count, CNOT count, circuit depth
- Circuit property extraction (T-count, gate counts, depth)

Supported input formats:
- DIMACS CNF (`.dimacs`): classical Tseitin encoding
- QDIMACS ECNF (`.qdimacs`): ESOP-style quantum encoding

---

# Repository Structure

```text
qSAT/
├── src/                        C++ source files
├── include/                    C++ headers
├── qsat/
│   └── Bindings.cpp            Python (pybind11) bindings
├── extern/                     Submodules (qcore, pybind11, ...)
├── test_input/
│   ├── dimacs/
│   │   ├── fun-bench/          Functional gate benchmarks (DIMACS)
│   │   ├── add-bench/          Adder benchmarks (DIMACS)
│   │   └── mul-bench/          Multiplier benchmarks (DIMACS)
│   └── qdimacs/
│       ├── fun-bench/          Functional gate benchmarks (QDIMACS)
│       ├── add-bench/          Adder benchmarks (QDIMACS)
│       └── mul-bench/          Multiplier benchmarks (QDIMACS)
├── scripts/
│   ├── generate_qasm.sh        DIMACS/QDIMACS -> OpenQASM
│   ├── decompose_qasm.sh       OpenQASM -> Clifford+T QASM
│   └── properties_qasm.sh      Extract circuit properties
├── CMakeLists.txt
├── pyproject.toml
├── setup.py
└── run_qasm_properties_batch.py
```

---

# Installation

## Clone Repository

```bash
git clone --recurse-submodules https://github.com/qc-agra-cps/qSAT.git
cd qSAT
```

If already cloned without submodules:

```bash
git submodule update --init --recursive
```

---

## Install in Python Environment

1. Create a virtual environment

        python3 -m venv .venv

2. Activate the environment

        source .venv/bin/activate

3. Install build tools

        pip install cmake ninja setuptools setuptools_scm

4. Install qSAT

        pip install .

5. Verify

        python3 -c "import pyqsat; print(pyqsat.__version__)"

---

# Usage

## Python API

```python
from pyqsat import *

# DIMACS/QDIMACS -> OpenQASM
dimacs_to_qasm_file("test_input/dimacs/fun-bench/AND_F__AND_R1_miter.dimacs",
                    "test_output/dimacs/fun-bench/AND_F__AND_R1_miter.qasm")

# OpenQASM -> Clifford+T decomposed QASM
decompose_qasm_file("test_output/dimacs/fun-bench/AND_F__AND_R1_miter.qasm",
                    "test_output/dimacs/fun-bench/decomposed_AND_F__AND_R1_miter.qasm")

# Extract circuit properties (T-count, gate counts, depth)
qasm_properties("test_output/dimacs/fun-bench/decomposed_AND_F__AND_R1_miter.qasm")

# From inline ECNF expression string (@ / * / xor as separator)
expr_to_qasm_file("(a ~b * c ~d) (b c d xor a) (~a ~c e)", "test_output/ecnf_expr_out.qasm")
```

---

## Shell Scripts

All three scripts accept `[dimacs|qdimacs]` as the first argument and an optional
`[fun-bench|add-bench|mul-bench|all]` as the second (default: `all`).

### 1. Generate OpenQASM from DIMACS/QDIMACS

Reads from `test_input/<format>/<bench>/`, writes to `test_output/<format>/<bench>/`.

```bash
./scripts/generate_qasm.sh qdimacs fun-bench
./scripts/generate_qasm.sh dimacs add-bench
./scripts/generate_qasm.sh qdimacs              # all benchmarks
```

### 2. Clifford+T Decomposition

Reads non-decomposed `.qasm` files from `test_output/<format>/<bench>/`,
writes `decomposed_<name>.qasm` alongside them.

```bash
./scripts/decompose_qasm.sh qdimacs fun-bench
./scripts/decompose_qasm.sh dimacs add-bench
./scripts/decompose_qasm.sh qdimacs mul-bench   # run separately, memory intensive
```

> **Note:** mul-bench decomposition (especially 8×8 and 9×9 multipliers) is memory
> intensive. Run it in isolation and by benchmark family.

### 3. Circuit Properties

Reads `decomposed_*.qasm` files from `test_output/<format>/<bench>/` and prints
gate summary for each.

```bash
./scripts/properties_qasm.sh qdimacs
./scripts/properties_qasm.sh dimacs fun-bench
```

---

# Input Format

qSAT accepts DIMACS CNF and QDIMACS ECNF files.

## DIMACS CNF

```text
p cnf <num_variables> <num_clauses>
c <signal comment>
<lit1> <lit2> ... 0
```

## QDIMACS ECNF

```text
p ecnf <num_variables> <num_clauses>    (or: p qcnf)
c <signal comment>
! @ <lit> @ <lit1> <lit2> 0    (complemented: NOT of XOR-of-products)
<lit> @ <lit1> <lit2> 0        (non-complemented: XOR-of-products)
<lit> 0                         (unit clause)
```

ECNF notation:

| Symbol     | Meaning                                         |
|------------|-------------------------------------------------|
| `@` / `*` / `xor` | monomial separator (AND-product boundary) |
| `!`        | clause complement (NOT output)                  |
| `-<n>`     | negated literal (same convention as DIMACS CNF) |

Literals within a `@`-delimited group are ANDed (one product).
`@` separates products. The clause is their XOR, optionally negated by `!`.

---

# Requirements

- C++20 compiler
- CMake ≥ 3.14
- Python ≥ 3.8
- ninja
- cmake, setuptools, setuptools_scm (Python build tools)
- qcore submodule (see `extern/`)

---

# License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

See the [LICENSE](LICENSE) file for the full license text.

---

# Authors

- Abhoy Kole
- Majd Assaad

Developed at:

Deutsches Forschungszentrum für Künstliche Intelligenz (DFKI)  
Cyber-Physical Systems Department

---
