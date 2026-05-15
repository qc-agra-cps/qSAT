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
 *  @file   Monomial.hpp
 *  @brief  Interface for representing a boolean conjunction (monomial).
 *  @details
 *  Defines the structure and behavior of a monomial — a conjunction
 *  of literals — used in qSAT’s CNF/ECNF representation.
 *  Each literal in a monomial is unique by its index
 *  (regardless of complement state). A monomial can be
 *  converted into a quantum circuit representation (MCX gate).
 *
 *  @date   29.04.2025
 *  @version 1.0
 *  @author Abhoy Kole
 ***********************************************************/

#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "Definition.hpp"
#include "Literal.hpp"
//#include "QCircuit.hpp"

namespace qSAT {

/**
 * @class Monomial
 * @brief Represents a conjunction (AND) of literals in a boolean formula.
 * @details
 * A monomial consists of a set of unique literals that are combined
 * conjunctively. Each literal appears only once by index (complemented
 * or not). This class provides functionality for parsing, merging,
 * cloning, and converting the monomial into a corresponding
 * quantum circuit representation.
 */
class Monomial {
   private:
    /** Collection of literals forming the conjunction. */
    LiteralSet literals;

   public:
    /** @brief Default constructor. Initializes an empty monomial. */
    Monomial();

    /**
     * @brief Copy constructor.
     * @param monomial The monomial instance to copy from.
     */
    Monomial(const Monomial& monomial);

    /**
     * @brief Copy assignment operator.
     * @param monomial The monomial instance to copy from.
     * @return Reference to the updated monomial.
     */
    Monomial& operator=(const Monomial& monomial);

    /**
     * @brief Constructs a monomial from a given set of literals.
     * @param literals Vector of unique literal pointers forming the conjunction.
     */
    explicit Monomial(const LiteralSet& literals);

    /**
     * @brief Constructs a monomial from a single literal.
     * @param literal The literal used to initialize the monomial.
     */
    explicit Monomial(const Literal& literal) {
        // Minimal fix: avoid initializer_list copies for unique_ptr literals.
        literals.emplace_back(LiteralPtr(new Literal(literal)));
    }

    /**
     * @brief Checks whether the monomial is valid.
     * @return True if the monomial contains valid literals, false otherwise.
     */
    bool isValid();


    /**
     * @brief Merges another monomial into this one.
     * @param other The monomial whose literals will be merged into this one.
     */
    void merge(const Monomial& other);

    /**
     * @brief Clones the contents of another monomial.
     * @param monomial The source monomial to clone.
     */
    void clone(const Monomial& monomial);

    /**
     * @brief Returns the string representation of the monomial.
     * @return A formatted string describing the conjunction.
     */
    std::string toString() const;

    /**
     * @brief Stream insertion operator for Monomial.
     * @param os Output stream.
     * @param monomial The monomial to print.
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Monomial& monomial);

    /**
     * @brief Converts this monomial into a corresponding quantum circuit.
     * @param parameters Parameter set describing circuit configuration.
     * @param target The target qubit for the operation.
     * @return A QCircuit representing this monomial.
     */
    // qcore::QCircuit toQCircuit(ParameterSet &parameters, const qcore::Qubit &target);

    /**
     * @brief Concatenates two literal sets by deep-copying elements.
     * @param v1 Target vector to be extended.
     * @param v2 Source vector whose elements will be appended.
     */
    friend void concatenate<Literal>(LiteralSet& v1, const LiteralSet& v2);

    /**
     * @brief Read-only access to the literals forming this monomial.
     */
    inline const LiteralSet& getLiterals() const { return literals; }
};

}  // namespace qSAT
