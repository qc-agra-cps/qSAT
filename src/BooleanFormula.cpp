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
 *  @file   BooleanFormula.cpp
 *  @brief  Implementation of a boolean function
 *  @author Abhoy Kole
 *  @date   02.10.2023
 ***********************************************************/

#include "BooleanFormula.hpp"
#include "Clause.hpp" 
#include "Literal.hpp"
#include "Monomial.hpp"

using namespace std;

namespace qSAT{
std::string BooleanFormula::toString() const {
    std::ostringstream oss;

    oss << "# Boolean Formula (" 
        << (parameters.clauseType == ClauseFormat::ECNF ? "ECNF" : "CNF") 
        << ")\n";
    oss << "# Inputs: " << parameters.inputCount 
        << ", Clauses: " << parameters.clauseCount << "\n";

    for (const auto &clausePtr : clauses) {
        if (clausePtr) {
            oss << clausePtr->toString() << "\n";
        }
    }

    return oss.str();
}

inline std::ostream& operator<<(std::ostream& os, const BooleanFormula& formula) {
    return os << formula.toString();
}


};
