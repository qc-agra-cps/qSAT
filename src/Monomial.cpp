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
 *  @file   Monomial.cpp
 *  @brief  Implementation of the Monomial class representing a conjunction of literals.
 *  @details
 *  A Monomial is a product (logical AND) of one or more literals. It forms
 *  the building block of ECNF clauses,
 *  where each monomial represents an AND-term and multiple monomials are
 *  combined with XOR operations at the clause level.
 *
 *  This implementation provides cloning, merging, and string serialization
 *  utilities used by the parser and the test framework to verify correctness
 *  of parsed formulas.
 *
 *  @see Monomial.hpp, Literal.hpp
 *  @author Majd Assaad
 *  @date   30.10.2025
 */

#include "Monomial.hpp"

#include <sstream>

#include "Literal.hpp"

// Minimal fix for tests: provide Monomial behaviour needed to inspect parser output.

namespace qSAT {

namespace {

/**
 * @brief Creates a deep copy of a Literal pointer.
 * @param literal Source literal pointer (may be null).
 * @return Newly allocated copy of the literal, or nullptr if input is null.
 */
LiteralPtr cloneLiteral(const LiteralPtr& literal) {
    if (!literal) {
        return nullptr;
    }
    return LiteralPtr(new Literal(*literal));
}

}  // namespace

/**
 * @brief Default constructor.
 * @details
 * Constructs an empty monomial with no literals.
 * An empty monomial is considered invalid until populated.
 */
Monomial::Monomial() = default;

/**
 * @brief Copy constructor.
 * @param monomial Source monomial to copy.
 * @details
 * Performs a deep copy of all contained literals.
 */
Monomial::Monomial(const Monomial& monomial) {
    clone(monomial);
}



/**
 * @brief Copy assignment operator.
 * @param monomial Source monomial to assign from.
 * @return Reference to this monomial.
 * @details
 * Performs deep copy of literal pointers to ensure independent ownership.
 */
Monomial& Monomial::operator=(const Monomial& monomial) {
    if (this != &monomial) {
        clone(monomial);
    }
    return *this;
}

/**
 * @brief Constructs a monomial from a set of literal pointers.
 * @param literalSet Container of LiteralPtr elements to copy.
 * @details
 * Creates a deep copy of each literal in the input set.
 */
Monomial::Monomial(const LiteralSet& literalSet) {
    literals.reserve(literalSet.size());
    for (const auto& lit : literalSet) {
        literals.emplace_back(cloneLiteral(lit));
    }
}

/**
 * @brief Validity check.
 * @return True if the monomial contains at least one valid literal; false otherwise.
 * @details
 * Each contained literal must be non-null and have a non-zero index.
 */
bool Monomial::isValid() {
    if (literals.empty()) {
        return false;
    }
    for (const auto& literal : literals) {
        if (!literal || !literal->isValid()) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Merges another monomial’s literals into this one.
 * @param other Another monomial whose literals will be appended.
 * @details
 * This operation concatenates the two literal lists, preserving order.
 * Used internally to combine adjacent AND terms.
 */
void Monomial::merge(const Monomial& other) {
    concatenate(literals, other.literals);
}

/**
 * @brief Clones all literals from another monomial.
 * @param monomial Source monomial to clone.
 * @details
 * Clears current literals and deep-copies each literal from the source.
 */
void Monomial::clone(const Monomial& monomial) {
    literals.clear();
    literals.reserve(monomial.literals.size());
    for (const auto& lit : monomial.literals) {
        literals.emplace_back(cloneLiteral(lit));
    }
}

/**
 * @brief Converts the monomial into a human-readable string.
 * @details
 * Literals are concatenated with " & " to indicate logical AND.
 * For example, the monomial (¬x1 ∧ x2) is rendered as "!x1 & x2".
 * @return String representation of the monomial.
 */
std::string Monomial::toString() const {
    std::ostringstream oss;
    for (size_t i = 0; i < literals.size(); ++i) {
        if (literals[i]) {
            oss << literals[i]->toString();
        }
        if (i + 1 < literals.size()) {
            oss << " & ";
        }
    }
    return oss.str();
}

/**
 * @brief Stream insertion operator for monomials.
 * @param os Output stream.
 * @param monomial Monomial to serialize.
 * @return Reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const Monomial& monomial) {
    return os << monomial.toString();
}

};  // namespace qSAT
