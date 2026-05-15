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
 *  @file Definition.hpp
 *  @brief Core type definitions and utilities for the QsatSolver package.
 *  @details 
 *  This file defines shared data types, forward declarations, and helper
 *  utilities used throughout the QsatSolver framework. It provides type
 *  aliases, format enumerations, and utility templates for managing
 *  literals, clauses, and CNF/ECNF structures.
 * 
 *  @date 23.04.2025
 *  @version 1.0
 *  @author Abhoy Kole
 ***********************************************************/

#pragma once

#include <bitset>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>


namespace qSAT {

/**
 * @class QSATException
 * @brief Exception class for handling qSAT-specific errors.
 * @details
 * Extends std::invalid_argument to provide more descriptive error messages
 * related to qSAT parsing and logic operations.
 */
class QSATException : public std::invalid_argument {
    /** @brief Detailed error message for the exception. */
    std::string msg;

   public:
    /**
     * @brief Constructs a QSATException with a given message.
     * @param m Error message describing the cause of the exception.
     */
    explicit QSATException(std::string m) : std::invalid_argument("qSAT Exception"), msg(std::move(m)) {}

    /**
     * @brief Returns the descriptive error message.
     * @return A C-style string containing the message.
     */
    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }
};

/**
 * @enum ClauseFormat
 * @brief Supported Boolean clause types and file formats.
 * @details
 * Enumerates possible input/output representations of Boolean formulas
 * used by the solver.
 */
enum class ClauseFormat {
    DIMACS,      ///< Standard CNF DIMACS format.
    QDIMACS,     ///< Quantum CNF DIMACS format.
    CNF,         ///< Conjunctive Normal Form.
    ECNF,        ///< Exclusive CNF (ESOP representation).
    FormatCount  ///< Number of supported formats.
};

using clauseFormat_t = ClauseFormat;
using count_t = std::uint32_t;

using index_t = std::uint64_t;

using index_map = std::map<index_t, index_t>;

// -----------------------------------------------------------------------------
// Forward Declarations and Smart Pointer Aliases
// -----------------------------------------------------------------------------

/** @brief Forward declaration of the Literal class. */
class Literal;
using LiteralPtr = std::unique_ptr<Literal>;
using LiteralSet = std::vector<LiteralPtr>;
using LiteralIterator = LiteralSet::iterator;
using ConstantLiteralIterator = LiteralSet::const_iterator;

/** @brief Forward declaration of the Monomial class. */
class Monomial;
using MonomialPtr = std::unique_ptr<Monomial>;
using MonomialSet = std::vector<MonomialPtr>;
using MonomialIterator = MonomialSet::iterator;
using ConstantMonomialIterator = MonomialSet::const_iterator;

/**
 * @typedef Expression
 * @brief Variant type that can hold either a Literal or Monomial pointer.
 */
using Expression = std::variant<LiteralPtr, MonomialPtr>;
using ExpressionSet = std::vector<Expression>;
using ExpressionIterator = ExpressionSet::iterator;
using ConstantExpressionIterator = ExpressionSet::const_iterator;

/** @brief Forward declaration of the Clause class. */
class Clause;
using ClausePtr = std::unique_ptr<Clause>;
using ClauseSet = std::vector<ClausePtr>;
using ClauseIterator = ClauseSet::iterator;
using ConstantClauseIterator = ClauseSet::const_iterator;

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------

/**
 * @brief Concatenates one vector of unique pointers to another.
 * @details
 * Performs a deep copy of the contents from the second vector and appends
 * them to the first. Each element in the destination vector will be a new
 * allocation cloned from the corresponding element of the source.
 * @tparam V Type of the object pointed to by the unique pointers.
 * @param v1 The destination vector to which elements will be added.
 * @param v2 The source vector whose elements will be cloned and appended.
 */

template <typename V>
void concatenate(std::vector<std::unique_ptr<V>>& v1, const std::vector<std::unique_ptr<V>>& v2) {
    // v1.insert(v1.end(), v2.begin(), v2.end());
    for (typename std::vector<std::unique_ptr<V>>::const_iterator l = v2.begin(); l != v2.end(); ++l) {
        v1.push_back(std::unique_ptr<V>(new V(**l)));
    }
}

};  // namespace qSAT
