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
 *  @file   ParameterSet.hpp
 *  @brief  Core type definitions and parameter utilities for qSAT Solver.
 *  @details 
 *  Defines the ParameterSet class, which encapsulates meta-information 
 *  about Boolean formulas such as input count, clause count, unit count, 
 *  and format type (CNF or ECNF). This serves as a foundational component 
 *  shared across the qSAT framework.
 * 
 *  @date   23.04.2025
 *  @version 1.0
 *  @author  Abhoy Kole
 ***********************************************************/

#pragma once

#include "Definition.hpp"

namespace qSAT {

/**
 * @class ParameterSet
 * @brief Represents the meta-parameters of a Boolean formula instance.
 * @details
 * Encapsulates structural information about a formula, including:
 * - Number of input literals
 * - Clause or polynomial count
 * - Unit clause count
 * - Clause format type (CNF or ECNF)
 * 
 * The ParameterSet is typically parsed from the header section of a 
 * DIMACS or QDIMACS file and used to guide subsequent clause parsing 
 * and circuit generation.
 */
class ParameterSet {
public:
    /** @brief Number of input literals. */
    count_t inputCount;         

    /** @brief Clause (CNF or qCNF) count. */
    count_t clauseCount;        

    /** @brief Number of unit clauses. */
    count_t unitCount;           

    /** @brief CNF vs. ECNF type. */
    clauseFormat_t clauseType;   
};

};  // namespace qSAT
