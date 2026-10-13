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
 *  @file   QCoreBridge.cpp
 *  @brief  Bridge between qSAT and qcore
 *  @details 
 * qSAT <-> qcore bridge
 * Converts parsed Boolean formulas (CNF/ECNF) into qcore::QCircuit
 * instances. The emitted circuit evaluates the formula on the input
 * qubits and writes the result to a single output qubit that follows
 * the clause ancillae.
 * 
 *  @author Majd Assaad
 *  @date   26.11.2025
 ***********************************************************/

#include "BooleanFormula.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "Clause.hpp"
#include "Literal.hpp"
#include "Monomial.hpp"
#include "ParameterSet.hpp"

#include <GateType.hpp>
#include <QCircuit.hpp>
#include <QGate.hpp>

namespace qSAT {

namespace {

using qcore::ControlSet;
using qcore::GateType;
using qcore::QGate;
using qcore::QCircuit;
using qcore::TargetSet;

struct GateBuilder {
    QCircuit& circuit;
	const qcore::DecompositionConfiguration &config;
    std::vector<qcore::QGate>* recorder{nullptr};

    void normaliseLastGate() {
        auto& gates = circuit.getGates();
        if (!gates.empty() && gates.back()) {
            gates.back()->setClassical(false);
        }
    }

    void addGate(QGate gate) {
        gate.setClassical(false);
        if (recorder) {
            recorder->push_back(gate);
        }
        circuit.addQGate(gate);
        normaliseLastGate();
    }

    void x(qcore::Qubit qubit) {
        addGate(QGate(GateType::X, 1, TargetSet{qubit}));
    }

    void h(qcore::Qubit qubit) {
        addGate(QGate(GateType::H, 1, TargetSet{qubit}));
    }

    void controlledX(const ControlSet& controls, qcore::Qubit target) {
        if (controls.empty()) {
            x(target);
            return;
        }

		qcore::QubitSet dirty_ancilla;
        std::copy_if(circuit.getQubits().begin(), circuit.getQubits().end(), std::back_inserter(dirty_ancilla),
                     [controls, target](qcore::Qubit q) {
                         return q != target &&
                                std::find(controls.begin(), controls.end(), q) == controls.end();
					 });
		auto decomposition = qcore::decompose_resolve_Toffoli(config, controls, target, {}, dirty_ancilla);
        for (auto const &gate : decomposition.getGates()) {
		  addGate(*gate);
		}
        // if (controls.size() == 1) {
        //     addGate(QGate(GateType::CX, 2, controls, TargetSet{target}));
        // } else if (controls.size() == 2) {
        //     addGate(QGate(GateType::CCX, 3, controls, TargetSet{target}));
        // } else {
        //     const auto gateSize = static_cast<qcore::gsize_t>(controls.size() + 1);
        //     addGate(QGate(GateType::MCX, gateSize, controls, TargetSet{target}));
        // }
    }
};

qcore::Qubit mapLiteralIndex(const Literal& literal, qcore::regsize_t inputCount) {
    const index_t idx = literal.getIndex();
    if (idx == 0 || idx > inputCount) {
        throw QSATException("Literal index " + std::to_string(idx) + " out of range for circuit generation.");
    }
    return static_cast<qcore::Qubit>(idx - 1);  // qcore uses 0-based qubit indices
}

/**
 * @brief Switches the given target qubit when all literals in the set are true.
 * @details Complements are handled by temporarily flipping the corresponding
 *          variable qubits so that qcore's positive controls can be used.
 */
void toggleOnMonomial(const LiteralSet& literals,
                      qcore::regsize_t inputCount,
                      qcore::Qubit target,
                      GateBuilder& builder) {
    if (literals.empty()) {
        throw QSATException("Cannot encode empty monomial in QCircuit.");
    }

    ControlSet controls;
    controls.reserve(literals.size());

    std::vector<qcore::Qubit> toggledQubits;
    toggledQubits.reserve(literals.size());

    for (const auto& literalPtr : literals) {
        if (!literalPtr) {
            throw QSATException("Null literal encountered while encoding monomial.");
        }
        const auto qubit = mapLiteralIndex(*literalPtr, inputCount);
        controls.push_back(qubit);

        // For complemented literals, flip the qubit so control fires on logical 1
        if (literalPtr->isComplemented()) {
            toggledQubits.push_back(qubit);
        }
    }

    for (const auto& qubit : toggledQubits) {
        builder.x(qubit);
    }

    builder.controlledX(controls, target);

    for (const auto& qubit : toggledQubits) {
        builder.x(qubit);
    }
}

/**
 * @brief Computes a CNF clause as an OR of its literals.
 * @details Uses De Morgan's law: clause = NOT(AND(all literals are false))
 *          Clause ancilla is initialised to |1> and flipped to |0> only when
 *          every literal evaluates to false.
 */
void emitCnfClause(const Clause& clause,
                   qcore::regsize_t inputCount,
                   qcore::Qubit clauseQubit,
                   GateBuilder& builder) {
    const auto& expressions = clause.getExpressions();
    if (expressions.empty()) {
        throw QSATException("Cannot encode empty CNF clause.");
    }

    ControlSet controls;
    std::vector<qcore::Qubit> toggledQubits;

    for (const auto& expr : expressions) {
        std::visit(
            [&](const auto& ptr) {
                using PtrType = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<PtrType, LiteralPtr>) {
                    if (!ptr) {
                        throw QSATException("Null literal encountered while encoding CNF clause.");
                    }
                    const auto qubit = mapLiteralIndex(*ptr, inputCount);
                    controls.push_back(qubit);
                    // For non-complemented literals, flip so the control fires when the literal is false.
                    // For complemented literals, the control already fires on the literal being false (variable = 1).
                    if (!ptr->isComplemented()) {
                        toggledQubits.push_back(qubit);
                    }
                } else if constexpr (std::is_same_v<PtrType, MonomialPtr>) {
                    throw QSATException("Monomials inside CNF clauses are not supported for circuit generation.");
                }
            },
            expr);
    }

    // Start at |1> so the clause becomes |0> only if every literal is false.
    builder.x(clauseQubit);
    for (const auto& qubit : toggledQubits) {
        builder.x(qubit);
    }
    builder.controlledX(controls, clauseQubit);
    for (const auto& qubit : toggledQubits) {
        builder.x(qubit);
    }

    if (clause.isComplementedClause()) {
        builder.x(clauseQubit);
    }
}

/**
 * @brief Computes an ECNF clause as an XOR of monomials.
 * @details Each monomial toggles the clause ancilla when it evaluates to true.
 */
void emitEcnfClause(const Clause& clause,
                    qcore::regsize_t inputCount,
                    qcore::Qubit clauseQubit,
                    GateBuilder& builder) {
    const auto& expressions = clause.getExpressions();
    if (expressions.empty()) {
        throw QSATException("Cannot encode empty ECNF clause.");
    }

    for (const auto& expr : expressions) {
        std::visit(
            [&](const auto& ptr) {
                using PtrType = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<PtrType, LiteralPtr>) {
                    if (!ptr) {
                        throw QSATException("Null literal encountered while encoding ECNF clause.");
                    }
                    LiteralSet singleton;
                    singleton.emplace_back(std::make_unique<Literal>(*ptr));
                    toggleOnMonomial(singleton, inputCount, clauseQubit, builder);
                } else if constexpr (std::is_same_v<PtrType, MonomialPtr>) {
                    if (!ptr) {
                        throw QSATException("Null monomial encountered while encoding ECNF clause.");
                    }
                    toggleOnMonomial(ptr->getLiterals(), inputCount, clauseQubit, builder);
                }
            },
            expr);
    }

    if (clause.isComplementedClause()) {
        builder.x(clauseQubit);
    }
    }

}  // namespace

QCircuit BooleanFormula::toQCircuit(qcore::DecompositionConfiguration config, MiterType m_type) const {
    const auto inputCount = static_cast<qcore::regsize_t>(parameters.inputCount);
    const auto clauseCount = static_cast<qcore::regsize_t>(clauses.size());

    if (inputCount == 0) {
        throw QSATException("Cannot create QCircuit: inputCount is zero.");
    }
    if (clauseCount == 0) {
        throw QSATException("Cannot create QCircuit: no clauses parsed.");
    }

    const bool phaseOracle = (m_type == MiterType::U_TYPE);
    if (m_type != MiterType::U_TYPE && m_type != MiterType::V_TYPE) {
        throw QSATException("Unsupported miter type for QCircuit generation.");
    }

    const qcore::regsize_t clauseBase = inputCount;
    const qcore::regsize_t outputQubit = clauseBase + clauseCount;
    const qcore::regsize_t totalQubits = phaseOracle ? outputQubit : outputQubit + 1;
    QCircuit circuit(static_cast<qcore::regsize_t>(totalQubits), 0);

    std::vector<qcore::QGate> computeGates;
    GateBuilder computeBuilder{circuit, config, phaseOracle ? &computeGates : nullptr};

    for (std::size_t i = 0; i < clauses.size(); ++i) {
        if (!clauses[i]) {
            throw QSATException("Null clause encountered while generating QCircuit.");
        }

        const qcore::Qubit clauseQubit = clauseBase + static_cast<qcore::Qubit>(i);
        const Clause& clause = *clauses[i];
        if (clause.getFormat() == ClauseFormat::ECNF) {
            emitEcnfClause(clause, inputCount, clauseQubit, computeBuilder);
        } else {
            emitCnfClause(clause, inputCount, clauseQubit, computeBuilder);
        }
    }

    if (phaseOracle) {
        // Phase mark solutions on the last clause ancilla and clean up.
        const qcore::Qubit phaseTarget = clauseBase + clauseCount - 1;
        ControlSet controls;
        controls.reserve(clauseCount > 0 ? clauseCount - 1 : 0);
        for (qcore::Qubit c = 0; c + 1 < clauseCount; ++c) {
            controls.push_back(clauseBase + c);
        }

        GateBuilder marker{circuit, config};
        marker.h(phaseTarget);
        marker.controlledX(controls, phaseTarget);
        marker.h(phaseTarget);

        GateBuilder uncompute{circuit, config};
        for (auto it = computeGates.rbegin(); it != computeGates.rend(); ++it) {
            uncompute.addGate(*it);
        }
    } else {
        // Combine clause ancillae with an AND into the final output qubit
        ControlSet controls;
        controls.reserve(clauseCount);
        for (qcore::Qubit c = 0; c < clauseCount; ++c) {
            controls.push_back(clauseBase + c);
        }

        GateBuilder builder{circuit, config};
        builder.controlledX(controls, outputQubit);
    }

    return circuit;
}

}  // namespace qSAT
