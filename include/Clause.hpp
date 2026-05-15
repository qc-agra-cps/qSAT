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
 *  @brief  Interface for representing a boolean clause (disjunction) in a CNF/ECNF formula.
 *  @details
 *  This file defines the Clause class, which models a disjunctive
 *  component of a boolean formula. Each Clause consists of a set of
 *  expressions (either literals or monomials) combined via OR operations.
 *  It can be parsed, validated, and converted into an equivalent
 *  quantum circuit representation.
 *
 *  @date   21.04.2025
 *  @version 1.0
 *  @author Abhoy Kole
 ***********************************************************/

#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "Definition.hpp"
//#include "QCircuit.hpp"

namespace qSAT {

/**
 * @class Clause
 * @brief Represents a single clause (disjunction) in a boolean formula.
 *
 * A Clause is a logical OR of multiple expressions, where each expression
 * may represent a literal or a monomial. Clauses are the fundamental units
 * of CNF and ECNF representations. The class provides mechanisms for parsing,
 * validation, deep cloning, and conversion into quantum circuit form.
 */
class Clause {
   public:
    /** @brief Default constructor. Initializes an empty clause. */
    Clause() = default;

    /**
     * @brief Constructs a clause from a given set of expressions.
     * @param t_expressions The set of expressions forming the clause.
     */
    explicit Clause(const ExpressionSet& t_expressions, ClauseFormat format = ClauseFormat::CNF, bool complemented=false, int index=-1);

    /**
     * @brief Constructs a clause by moving in a set of expressions with a specified format.
     * @param t_expressions The set of expressions forming this clause (moved for efficiency).
     * @param format The clause format (e.g., CNF or ECNF).
     */
    Clause(ExpressionSet&& t_expressions, ClauseFormat format = ClauseFormat::CNF) noexcept
        : index(-1),
          complemented(false),
          clauseFormat(format),
          expressions(std::move(t_expressions)) {}

    /**
     * @brief Constructs a clause from a single expression (rvalue reference).
     * @param t_expression The expression to move into the clause.
     */
    explicit Clause(Expression&& t_expression) {
        // Minimal fix: avoid initializer_list copies for non-copyable variants.
        clauseFormat = ClauseFormat::CNF;
        expressions.emplace_back(std::move(t_expression));
    }

    /**
     * @brief Constructs a clause from a single expression (deep copy).
     * @param t_expression The expression to copy and encapsulate.
     */
    explicit Clause(const Expression& t_expression) : Clause(ExpressionSet(), ClauseFormat::CNF) {
        expressions.emplace_back(cloneExpression(t_expression));
    }

    /**
     * @brief Checks whether the clause is valid.
     * @return True if the clause is structurally valid, false otherwise.
     */
    bool isValid();

    /**
     * @brief Add an expression (literal or monomial) to the clause by moving it in.
     * @param expr Expression to add.
     */
    void addExpression(Expression&& expr);

    /**
     * @brief Add an expression by copying/cloning the provided expression.
     * @param expr Expression to add (copied).
     */
    void addExpression(const Expression& expr);

    /**
     * @brief Convenience: add a literal given as a signed integer.
     * @param literalValue Signed literal value (e.g. -3 or 5).
     */
    void addLiteral(int literalValue);

      /**
     * @brief Convenience: add a monimial given ...
     * @param literalValue Signed literal values (e.g. -3 5 ... or 3 -5 ...).
     */
    //void addMonimial(??);

    /**
     * @brief Check whether the clause contains no expressions.
     * @return true if empty.
     */
    [[nodiscard]] bool empty() const noexcept {
        return expressions.empty();
    }

    /**
     * @brief Number of expressions in this clause.
     */
    [[nodiscard]] size_t size() const noexcept {
        return expressions.size();
    }

    /**
     * @brief Returns the stored clause format (CNF/ECNF).
     */
    inline ClauseFormat getFormat() const { return clauseFormat; }

    /**
     * @brief Returns whether this clause is complemented.
     */
    inline bool isComplementedClause() const { return complemented; }

    /**
     * @brief Returns the clause index (if set).
     */
    inline int getIndex() const { return index; }

    /**
     * @brief Sets whether the clause is complemented.
     */
    inline void setComplemented(bool value) { complemented = value; }

    /**
     * @brief Toggles the complemented flag.
     */
    inline void flipComplemented() { complemented = !complemented; }

    /**
     * @brief Read-only access to the clause expressions.
     */
    inline const ExpressionSet& getExpressions() const { return expressions; }

    /**
     * @brief Returns a string representation of the clause.
     * @return A string encoding of the clause (e.g., "(x1 or x2 or !x3)" or "(x1 xor x2 xor !x3)").
     */
    std::string toString();

    /**
     * @brief Stream insertion operator for Clause.
     * @param os Output stream.
     * @param clause Clause instance to print.
     * @return Reference to the output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Clause& clause);

    /**
     * @brief Converts the clause into its corresponding quantum circuit.
     *
     * @param parameters Parameter set describing the circuit configuration.
     * @param offset The number of qubits between this index qubit and the target qubit.
     * @return A QCircuit representing this clause.
     */
    // qcore::QCircuit toQCircuit(ParameterSet &parameters, const int &offset = 0);

   private:
    /** @brief Index of the clause in the formula (for reference or ordering). */
    int index = -1;

    /** @brief Whether this clause represents a complemented (negated) form. */
    bool complemented = false;

    /** @brief Whether this clause represents a CNF or ECNF. */
    ClauseFormat clauseFormat;

    /** @brief Collection of expressions (literals or monomials) forming this clause. */
    ExpressionSet expressions;

    /**
     * @brief Deep clones an expression (either a LiteralPtr or MonomialPtr).
     * @param expr The expression to clone.
     * @return A new expression with a deep copy of the underlying object.
     */
    static Expression cloneExpression(const Expression& expr);
};

}  // namespace qSAT
