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
 *  @file   Literal.hpp
 *  @brief  Defines the interface for representing a boolean literal.
 *  @details
 *  A literal corresponds to a single boolean variable or its negation,
 *  and can be represented as an X gate in a quantum circuit. This class
 *  provides parsing, conversion, and string representation utilities
 *  for literals used in CNF expressions.
 *
 *  @date 28.04.2025
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
 * @class Literal
 * @brief Represents a single boolean literal in a CNF formula.
 * @details
 * A literal is defined by an index (variable ID) and a complement flag
 * indicating whether it represents the variable itself or its negation.
 * The class supports parsing from textual input, validity checking,
 * and conversion into a quantum circuit representation.
 */
class Literal {
   private:
    /** @brief Index of the literal (unique identifier). */
    index_t index;

    /** @brief True if the literal is complemented (negated). */
    bool complemented;

   public:
    /**
     * @brief Default constructor.
     * @details Initializes an invalid literal with undefined index and no complement.
     */
    Literal();

    /**
     * @brief Copy constructor.
     * @param other The literal to copy.
     */
    Literal(const Literal& other);

    /**
     * @brief Copy assignment operator.
     * @param other The literal to copy from.
     * @return Reference to this literal.
     */
    Literal& operator=(const Literal& other);

    /**
     * @brief Constructs a literal with the given index and complement flag.
     * @param index The index of the literal (variable ID).
     * @param complemented Whether the literal is complemented (negated).
     */
    Literal(const index_t index, const bool complemented = false);

    /**
     * @brief Checks if the literal is valid.
     * @return True if the literal has a valid index; false otherwise.
     */
    bool isValid();

    /**
     * @brief Returns a string representation of the literal.
     * @return A string encoding of the literal (e.g., "x3" or "!x4").
     */
    std::string toString() const;

    /**
     * @brief Stream insertion operator for Literal.
     * @param os Output stream.
     * @param literal The literal to print.
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Literal& literal);
    
    /**
     * @brief Converts the literal into a quantum circuit representation.
     * @return A QCircuit object corresponding to the literal.
     * @details A non-complemented literal is represented as an X gate applied to its corresponding qubit index.
     */
    // qcore::QCircuit toQCircuit();

    /**
     * @brief Returns the index of the literal.
     * @return The index (variable identifier).
     */
    inline index_t getIndex() const { return this->index; }

    /**
     * @brief Checks whether the literal is complemented.
     * @return True if complemented; false otherwise.
     */
    inline bool isComplemented() const { return this->complemented; }
};
};  // namespace qSAT
