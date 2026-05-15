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
 *  @file   Decomposer.cpp
 *  @brief  
 *  @author Majd Assaad
 *  @date   29.12.2025
 ***********************************************************/

#include "Decomposer.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include <decompose/Clifford_T.hpp>

#include "Definition.hpp"

namespace {

qcore::regsize_t ancillasNeededForGate(qcore::QGate& gate) {
    if (gate.getType() != qcore::GateType::MCX) {
        return 0;
    }

    const auto controlCount = static_cast<qcore::regsize_t>(gate.getControls().size());
    if (controlCount <= 2) {
        return 0;
    }

    return ((controlCount - 2) / 2) + ((controlCount - 2) % 2);
}

void ensureSingleTarget(qcore::QGate& gate) {
    if (gate.getTargets().size() != 1) {
        throw qSAT::QSATException("Multi-controlled gate requires exactly one target qubit for decomposition.");
    }
}

qcore::QubitSet buildAncillaPool(qcore::regsize_t base, qcore::regsize_t count) {
    qcore::QubitSet pool;
    pool.reserve(count);
    for (qcore::regsize_t i = 0; i < count; ++i) {
        pool.push_back(static_cast<qcore::Qubit>(base + i));
    }
    return pool;
}

}  // namespace

namespace qSAT {

qcore::QCircuit decomposeToCliffordT(const qcore::QCircuit& circuit) {
    auto& gates = const_cast<qcore::QCircuit&>(circuit).getGates();

    qcore::regsize_t maxAncillas = 0;
    for (auto& gatePtr : gates) {
        if (!gatePtr) {
            continue;
        }
        maxAncillas = std::max(maxAncillas, ancillasNeededForGate(*gatePtr));
    }

    const auto ancillaBase = const_cast<qcore::QCircuit&>(circuit).getQregSize();
    qcore::QubitSet ancillaPool = buildAncillaPool(ancillaBase, maxAncillas);

    qcore::QCircuit decomposed(static_cast<qcore::regsize_t>(ancillaBase + maxAncillas),
                               const_cast<qcore::QCircuit&>(circuit).getCregSize());

    for (auto& gatePtr : gates) {
        if (!gatePtr) {
            continue;
        }

        auto& gate = *gatePtr;
        const auto type = gate.getType();
        if (type == qcore::GateType::MCX || type == qcore::GateType::CCX) {
            ensureSingleTarget(gate);
            const auto needed = ancillasNeededForGate(gate);
            if (needed > ancillaPool.size()) {
                throw QSATException("Insufficient ancillas allocated for MCX decomposition.");
            }
            const auto cleanCount = static_cast<qcore::regsize_t>((needed + 1) / 2);  // favor at least one clean
            const auto dirtyCount = needed >= cleanCount ? needed - cleanCount : 0;

            qcore::QubitSet cleanAncilla;
            qcore::QubitSet dirtyAncilla;
            cleanAncilla.insert(cleanAncilla.end(), ancillaPool.begin(), ancillaPool.begin() + cleanCount);
            dirtyAncilla.insert(dirtyAncilla.end(), ancillaPool.begin() + cleanCount, ancillaPool.begin() + cleanCount + dirtyCount);

            decomposed.addQCircuit(
                qcore::decompose_MCT_Clifford_T(gate.getControls(), gate.getTargets()[0], dirtyAncilla, cleanAncilla));
        } else {
            decomposed.addQGate(gate);
        }
    }

    return decomposed;
}

qcore::QCircuit decomposeQasmFile(const std::string& path) {
    qcore::QCircuit circuit;
    circuit.readQCircuit(path);
    return decomposeToCliffordT(circuit);
}

qcore::QCircuit decomposeQasmString(const std::string& qasm) {
    qcore::QCircuit circuit;
    std::istringstream is(qasm);
    circuit.readQASM(is);
    return decomposeToCliffordT(circuit);
}

}  // namespace qSAT
