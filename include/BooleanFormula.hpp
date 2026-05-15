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
 *  @file   BooleanFormula.hpp
 *  @brief  Interface for a Boolean formula (CNF or ECNF)
 *  @details
 *  Defines the BooleanFormula class which holds clauses,
 *  meta-parameters, and provides parsing and quantum circuit conversion.
 *  @date   21.04.2025
 *  @version 1.0
 *  @author Abhoy Kole
 ***********************************************************/

#pragma once

#include <fstream>

#include "Definition.hpp"
#include "Clause.hpp"
#include "ParameterSet.hpp"
#include "Literal.hpp"
#include "Monomial.hpp"
#include <QCircuit.hpp>

namespace qSAT {

/**
 * @enum MiterType
 * @brief Supported Miter Construction types.
 * @details
 * Enumerates possible miter representations of Boolean formulas
 * used by the solver. U_TYPE is the standard Grover Oracle,
 * V_TYPE is a low-cost verification miter.
 */
enum class MiterType {
    U_TYPE,     ///< Standard miter for Grover's algorithm Oracle.
    V_TYPE,     ///< Low-cost miter for SAT/UNSAT verification.
    MiterCount  ///< Number of supported miter types.
};

/**
 * @class BooleanFormula
 * @brief Represents a Boolean formula composed of multiple clauses.
 * @details
 * Stores the clauses and meta-parameters (input count, clause count, type)
 * of a Boolean formula. Provides functionality to parse formulas from files
 * and convert them into quantum circuits using U-type or V-type miters.
 */
class BooleanFormula {
   private:
    ParameterSet parameters;  ///< Meta-parameters of the formula.
    ClauseSet clauses;        ///< Collection of clauses forming the formula.

   public:
    /**
     * @brief Parses a Boolean formula from an input stream.
     * @details
     * Accepts any standard C++ input stream — file, string, or memory buffer.
     * This allows flexible parsing from various sources.
     *
     * @param input Input stream containing the Boolean formula.
     */
    void parseFrom(std::istream& input);

    /**
     * @brief Parses the Boolean formula from a file path.
     * @param filename Path to the file containing the formula.
     */
    void parseFrom(const std::string& filename);

    /**
     * @brief Parses the Boolean formula from a raw string.
     * @param formulaText String representation of the formula.
     */
    void parseFromString(const std::string& formulaText);

    /**
     * @brief Returns a string representation of the Boolean formula.
     * @details
     * Converts all contained clauses and literals into a human-readable
     * representation in CNF or ECNF format.
     *
     * @return A formatted string representing the Boolean formula.
     */
    std::string toString() const;

    /**
     * @brief Stream insertion operator for BooleanFormula.
     * @param os The output stream to which the formula will be written.
     * @param formula The BooleanFormula object to output.
     * @return Reference to the output stream, enabling chaining of << operators.
     */
    friend std::ostream& operator<<(std::ostream& os, const BooleanFormula& formula);

    /**
     * @brief Converts the formula into a quantum circuit representation.
     * @param m_type The type of miter to construct (default: U_TYPE).
     * @return QCircuit representing the Boolean formula.
     */
    qcore::QCircuit toQCircuit(MiterType m_type = MiterType::U_TYPE) const;
};

}  // namespace qSAT
