/*
 * This file is part of the qSAT Solver package.
 *
 * Developed for the Deutsches Forschungszentrum für Künstliche
 * Intelligenz GmbH (DFKI), Cyber-Physical Systems Dept.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 *  @file   Decomposer.hpp
 *  @brief  Helpers for expanding multi-controlled X gates using qcore's Clifford+T decomposition.
 *  @author Majd Assaad
 *  @date   28.12.2025
 ***********************************************************/

#pragma once

#include <QCircuit.hpp>

namespace qSAT {

/**
 * @brief Decompose all CCX/MCX gates in a circuit into the Clifford+T basis.
 *
 * Appends the minimum number of ancilla qubits required by the
 * decompose_MCT_Clifford_T implementation and reuses them across gates.
 * Ancillas are split between clean and dirty (roughly half/half, with at
 * least one clean).
 *
 * @param circuit The circuit to decompose.
 * @return A new circuit containing only Clifford+T gates for the multi-control X operations.
 */
qcore::QCircuit decomposeToCliffordT(const qcore::QCircuit& circuit);

/**
 * @brief Read a QASM file and return its Clifford+T decomposition.
 */
qcore::QCircuit decomposeQasmFile(const std::string& path);

/**
 * @brief Read a QASM string and return its Clifford+T decomposition.
 */
qcore::QCircuit decomposeQasmString(const std::string& qasm);

}  // namespace qSAT
