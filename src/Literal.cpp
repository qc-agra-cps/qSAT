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
 *  @file   Literal.cpp
 *  @brief  Implementation of the Literal class representing a Boolean variable.
 *  @details
 *  The Literal class is the smallest structural unit in a Boolean formula.
 *  Each literal represents a variable (identified by its index) and may
 *  optionally be complemented (negated). Literals are used within Monomials
 *  (AND terms) and Clauses (OR/XOR collections) throughout the parser and
 *  formula representation.
 *
 *  This implementation provides minimal construction, copying, and string
 *  serialization required by the parser and test utilities.
 *
 *  @see Literal.hpp
 *  @author Majd Assaad
 *  @date   30.10.2025
 */

#include "Literal.hpp"

// Minimal fix for tests: provide Literal definitions required by parser validation.

#include <sstream>

namespace qSAT {

Literal::Literal() : index(0), complemented(false) {}

Literal::Literal(const Literal& other) = default;

Literal& Literal::operator=(const Literal& other) = default;

Literal::Literal(const index_t idx, const bool comp) : index(idx), complemented(comp) {}

bool Literal::isValid() {
    return index != 0;
}

std::string Literal::toString() const {
    std::ostringstream oss;
    if (complemented) {
        oss << "!";
    }
    oss << "x" << index;
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Literal& literal) {
    return os << literal.toString();
}

};  // namespace qSAT
