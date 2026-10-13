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
 *  @file   Clause.cpp
 *  @brief  Implementation of the Literal class for boolean formula representation.
 *  @details
 *  Provides functionality for constructing, parsing, validating,
 *  and converting boolean literals into quantum circuit equivalents.
 *
 *  @date   28.04.2025
 *  @version 1.0
 *  @author Abhoy Kole
 ***********************************************************/

#include "Clause.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <set>
#include <optional>

#include "Literal.hpp"
#include "Monomial.hpp"

namespace qSAT {

Clause::Clause(Clause const &o) : Clause(o.expressions, o.getFormat(), o.complemented, o.index) {}

/**
 * @brief Constructs a clause by deeply copying a given set of expressions.
 * @param t_expressions The set of expressions to clone into this clause.
 * @param format The clause format (e.g., CNF or ECNF).
 * @param complemented Whether this clause is negated.
 * @param index The index of this clause within the Boolean formula.
 */
Clause::Clause(const ExpressionSet& t_expressions,
               ClauseFormat format,
               bool complemented,
               int index)
    : index(index),
      complemented(complemented),
      clauseFormat(format),
      expressions() {
    expressions.reserve(t_expressions.size());
    std::transform(
        t_expressions.begin(), t_expressions.end(),
        std::back_inserter(expressions),
        [](const Expression& expr) { return cloneExpression(expr); });
}

/**
 * @brief Checks whether the clause is structurally valid.
 * @return True if the clause is valid, false otherwise.
 */
bool Clause::isValid() {
    if (expressions.empty())
        return false;

    for (const auto& expr : expressions) {
        bool valid = std::visit([](const auto& ptr) -> bool {
            return ptr != nullptr;
        },
                                expr);
        if (!valid)
            return false;
    }
    return true;
}

/**
 * @brief Add an expression (literal or monomial) by moving it into the clause.
 * @param expr Expression to add (moved).
 */
void Clause::addExpression(Expression&& expr) {
    expressions.emplace_back(std::move(expr));
}

/**
 * @brief Add an expression (literal or monomial) by copying/cloning it.
 * @param expr Expression to add (copied).
 */
void Clause::addExpression(const Expression& expr) {
    expressions.emplace_back(cloneExpression(expr));
}

/**
 * @brief Convenience: add a literal given as a signed integer.
 * @param literalValue Signed literal value (e.g., -3 or 5).
 */
void Clause::addLiteral(int literalValue) {
    auto literalPtr = std::make_unique<Literal>(literalValue);
    expressions.emplace_back(std::move(literalPtr));
}

/**
 * @brief Creates a deep copy of an Expression (Literal or Monomial).
 * @param expr Expression variant to clone.
 * @return A new Expression containing a deep-copied Literal or Monomial.
 */
Expression Clause::cloneExpression(const Expression& expr) {
    return std::visit([](const auto& ptr) -> Expression {
        using PtrType = std::decay_t<decltype(ptr)>;
        if (ptr) {
            return PtrType(new typename PtrType::element_type(*ptr));
        }
        // Minimal fix: return an empty pointer variant instead of nullptr.
        return PtrType{};
    },
                      expr);
}

/**
 * @brief Returns a string representation of the clause.
 * @return A string encoding of the clause (e.g., "(x1 ∨ x2 ∨ ¬x3)").
 */
std::string Clause::toString() {
    std::ostringstream oss;
    oss << "(";
	if (complemented) {
	    oss << "1 * ";
    }
    for (size_t i = 0; i < expressions.size(); ++i) {
        std::visit([&oss](const auto& ptr) {
            if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, LiteralPtr>) {
                if (ptr) oss << ptr->toString();
            } else if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, MonomialPtr>) {
                if (ptr) oss << ptr->toString();
            }
        },
                   expressions[i]);

        // Append operator symbol between expressions
        if (i + 1 < expressions.size())
            oss << ((clauseFormat == ClauseFormat::ECNF) ? " * " : " + ");
    }

    oss << ")";
    return oss.str();
}

// Helper function
std::set<index_t> get_vars(Clause const &c) {
    std::set<index_t> vars;
    for (auto const &expr : c.getExpressions()) {
        std::visit(
            [&vars](auto const &expr) {
                using T = std::decay_t<decltype(expr)>;
                if constexpr (std::is_same_v<T, LiteralPtr>) {
                    vars.emplace(expr->getIndex());
                } else {
                    for (auto const &lit : expr->getLiterals()) {
                        vars.emplace(lit->getIndex());
                    }
                }
            },
            expr);
    }
    return vars;
};

/**
 * @brief Returns wether two clauses share at least one variable (ignoring negation)
 */
bool Clause::shares_variable_with(Clause const &other) const {
    auto vars_a = get_vars(*this);
    auto vars_b = get_vars(other);
    for (auto a : vars_a) {
        for (auto b : vars_b) {
            if (a == b)
                return true;
        }
    }
    return false;
}

std::size_t Clause::getNumLiterals() const {
    return get_vars(*this).size();
}

MonomialPtr &Clause::getMonomial() {
    std::optional<std::reference_wrapper<MonomialPtr>> result = {};
    for (auto &expr : expressions) {
        if (std::holds_alternative<MonomialPtr>(expr)) {
		  if (result && (*result).get()->getLiterals().size() > 1) {
                throw std::logic_error{"Clause contains more than one monomial"};
            }
            result = std::get<MonomialPtr>(expr);
        }
    }
    if (!result) {
        throw std::logic_error{"Clause does not contain a monomial"};
    }
    return *result;
}

MonomialPtr const &Clause::getMonomial() const {
    std::optional<std::reference_wrapper<MonomialPtr const>> result = {};
    for (auto const &expr : expressions) {
        if (std::holds_alternative<MonomialPtr>(expr)) {
		  if (result && (*result).get()->getLiterals().size() > 1) {
                throw std::logic_error{"Clause contains more than one monomial"};
            }
            result = std::get<MonomialPtr>(expr);
        }
    }
    if (!result) {
        throw std::logic_error{"Clause does not contain a monomial"};
    }
    return *result;
}

bool Clause::isSingleMonomial() const {
    if (expressions.size() > 1)
	  return false;
    return true;
}

/**
 * @brief Outputs a textual representation of the clause to a stream.
 * @code
 * qSAT::Clause c;
 * std::cout << c << std::endl;
 * @endcode
 *
 * @param os Output stream to write to.
 * @param clause The Clause instance to render.
 * @return Reference to the same output stream.
 */
std::ostream& operator<<(std::ostream& os, const Clause& clause) {
    os << "(";

    for (size_t i = 0; i < clause.expressions.size(); ++i) {
        std::visit([&os](const auto& ptr) {
            if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, qSAT::LiteralPtr>) {
                if (ptr) os << ptr->toString();
            } else if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, qSAT::MonomialPtr>) {
                if (ptr) os << ptr->toString();
            }
        },
                   clause.expressions[i]);

        // Append operator symbol between expressions
        if (i + 1 < clause.expressions.size())
            os << ((clause.clauseFormat == ClauseFormat::ECNF) ? " * " : " + ");
    }

    os << ")";
    return os;
}
}  // namespace qSAT
