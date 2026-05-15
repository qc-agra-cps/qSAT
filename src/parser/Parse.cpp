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
 *  @file   Parse.cpp
 *  @brief  Implementation of a boolean function
 *  @author Abhoy Kole
 *  @author Majd Assaad
 *  @date   28.11.2025
 ***********************************************************/
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "BooleanFormula.hpp"
#include "Clause.hpp"
#include "Literal.hpp"
#include "Monomial.hpp"

using namespace std;

namespace qSAT {

namespace {

/**
 * @brief Produces a trimmed copy of the supplied string.
 * @param text Input string that may contain leading or trailing whitespace.
 * @return String with surrounding whitespace removed.
 */
std::string trimCopy(const std::string& text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch); });
    if (first == text.end()) {
        return {};
    }
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return std::string(first, last);
}

/**
 * @brief Heuristically detects whether the provided content resembles DIMACS input.
 * @param content Entire file or buffer content.
 * @return True if a DIMACS header/literal line is discovered, otherwise false.
 */
bool isDimacsLike(const std::string& content) {
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        const auto trimmed = trimCopy(line);
        if (trimmed.empty()) {
            continue;
        }

        const char lead = static_cast<char>(std::tolower(trimmed.front()));
        if (lead == 'c' || lead == '#') {
            continue;
        }

        if (lead == 'p') {
            return true;
        }

        if (std::isdigit(lead) || lead == '-') {
            return true;
        }

        break;
    }

    return false;
}

/**
 * @brief Determines whether the DIMACS ECNF input uses double-zero encoding.
 * @param content Entire DIMACS stream to inspect.
 * @return True when any clause contains consecutive zero tokens, indicating double-zero style.
 */
bool detectEcnfDoubleZeroStyle(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    bool ecnfActive = false;

    while (std::getline(stream, line)) {
        const std::string sanitized = trimCopy(line);
        if (sanitized.empty()) {
            continue;
        }

        const char lead = static_cast<char>(std::tolower(sanitized.front()));
        if (lead == 'c' || lead == '#') {
            continue;
        }

        if (lead == 'p') {
            std::istringstream headerStream(sanitized);
            std::string label;
            std::string format;
            if (!(headerStream >> label >> format)) {
                continue;
            }
            std::transform(format.begin(), format.end(), format.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

            ecnfActive = (format == "ecnf" || format == "qcnf");
            continue;
        }

        if (!ecnfActive) {
            continue;
        }

        std::istringstream tokenStream(sanitized);
        std::string token;
        bool previousWasZero = false;

        while (tokenStream >> token) {
            if (token == "0") {
                if (previousWasZero) {
                    return true;
                }
                previousWasZero = true;
            } else {
                previousWasZero = false;
            }
        }
    }

    return false;
}

/**
 * @brief Extracts the textual contents of clauses enclosed in parentheses.
 * @param expression Raw symbolic expression string.
 * @return Vector containing the textual body of each bracketed clause.
 * @throws QSATException If parentheses are unbalanced or mismatched.
 */
std::vector<std::string> extractClauseBodies(const std::string& expression) {
    std::vector<std::string> clauses;
    std::string buffer;
    int depth = 0;

    for (char ch : expression) {
        if (ch == '(') {
            if (depth == 0) {
                buffer.clear();
            } else {
                buffer.push_back(ch);
            }
            ++depth;
        } else if (ch == ')') {
            if (depth == 0) {
                throw QSATException("Mismatched parenthesis while parsing expression string.");
            }
            --depth;
            if (depth == 0) {
                clauses.push_back(trimCopy(buffer));
            } else {
                buffer.push_back(ch);
            }
        } else if (depth > 0) {
            buffer.push_back(ch);
        }
    }

    if (depth != 0) {
        throw QSATException("Unclosed parenthesis in expression string.");
    }

    return clauses;
}

/**
 * @brief Splits a string using a single-character delimiter, trimming each token.
 * @param text Input string to split.
 * @param delimiter Delimiter character.
 * @return Vector with trimmed, non-empty tokens.
 */
std::vector<std::string> splitByDelimiter(const std::string& text, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;

    for (char ch : text) {
        if (ch == delimiter) {
            const auto trimmed = trimCopy(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    const auto trimmed = trimCopy(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    return tokens;
}

/**
 * @brief Splits a string using any character from a delimiter set, trimming each token.
 * @param text Input string to split.
 * @param delimiters Set of delimiter characters.
 * @return Vector with trimmed, non-empty tokens.
 */
std::vector<std::string> splitByAnyDelimiter(const std::string& text, const std::string& delimiters) {
    std::vector<std::string> tokens;
    std::string current;

    for (char ch : text) {
        if (delimiters.find(ch) != std::string::npos) {
            const auto trimmed = trimCopy(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    const auto trimmed = trimCopy(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    return tokens;
}

/**
 * @brief Splits a string on logical OR keywords ("or").
 * @return Empty vector if no keyword was found.
 */
std::vector<std::string> splitByOrKeyword(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    std::string current;
    bool found = false;

    while (iss >> token) {
        std::string lowered(token.size(), '\0');
        std::transform(token.begin(), token.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered == "or") {
            found = true;
            const auto trimmed = trimCopy(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            if (!current.empty()) current.push_back(' ');
            current += token;
        }
    }

    const auto trimmed = trimCopy(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    if (!found) {
        return {};
    }
    return tokens;
}

/**
 * @brief Splits a string on logical AND keywords ("and").
 * @return Empty vector if no keyword was found.
 */
std::vector<std::string> splitByAndKeyword(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    std::string current;
    bool found = false;

    while (iss >> token) {
        std::string lowered(token.size(), '\0');
        std::transform(token.begin(), token.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered == "and") {
            found = true;
            const auto trimmed = trimCopy(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            if (!current.empty()) current.push_back(' ');
            current += token;
        }
    }

    const auto trimmed = trimCopy(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    if (!found) {
        return {};
    }
    return tokens;
}

/**
 * @brief Splits a string on logical XOR keywords ("xor").
 * @return Empty vector if no keyword was found.
 */
std::vector<std::string> splitByXorKeyword(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    std::string current;
    bool found = false;

    while (iss >> token) {
        std::string lowered(token.size(), '\0');
        std::transform(token.begin(), token.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered == "xor") {
            found = true;
            const auto trimmed = trimCopy(current);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
            current.clear();
        } else {
            if (!current.empty()) current.push_back(' ');
            current += token;
        }
    }

    const auto trimmed = trimCopy(current);
    if (!trimmed.empty()) {
        tokens.push_back(trimmed);
    }

    if (!found) {
        return {};
    }
    return tokens;
}

/**
 * @brief Splits a string on ASCII whitespace characters.
 * @param text Input string to tokenise.
 * @return Vector of whitespace-delimited tokens.
 */
std::vector<std::string> splitByWhitespace(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

/**
 * @brief Infers clause format (CNF or ECNF) from symbolic clause strings.
 * @param clauses Tokenised clause bodies.
 * @param rawExpression Entire expression string for context hints.
 * @return Detected clause format, defaults to CNF when ambiguous.
 */
ClauseFormat detectClauseFormat(const std::vector<std::string>& clauses, const std::string& rawExpression) {
    std::size_t plusCount = 0;
    std::size_t starCount = 0;
    std::size_t atCount = 0;
    std::size_t xorCount = 0;
    std::size_t orKeywordCount = 0;

    for (const auto& clause : clauses) {
        plusCount += static_cast<std::size_t>(std::count(clause.begin(), clause.end(), '+'));
        starCount += static_cast<std::size_t>(std::count(clause.begin(), clause.end(), '*'));
        atCount += static_cast<std::size_t>(std::count(clause.begin(), clause.end(), '@'));

        std::istringstream iss(clause);
        std::string tok;
        while (iss >> tok) {
            std::string lowered(tok.size(), '\0');
            std::transform(tok.begin(), tok.end(), lowered.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lowered == "xor") {
                ++xorCount;
            } else if (lowered == "or") {
                ++orKeywordCount;
            }
        }
    }

    if ((starCount + atCount) > 0 && plusCount == 0) {
        return ClauseFormat::ECNF;
    }
    if (plusCount > 0 && starCount == 0) {
        return ClauseFormat::CNF;
    }

    if (xorCount > 0 && starCount == 0 && plusCount == 0) {
        return ClauseFormat::ECNF;
    }
    if (orKeywordCount > 0 && plusCount == 0 && starCount == 0 && xorCount == 0) {
        return ClauseFormat::CNF;
    }

    const auto prefixEnd = rawExpression.find('(');
    if (prefixEnd != std::string::npos) {
        const std::string prefix = trimCopy(rawExpression.substr(0, prefixEnd));
        std::string lowered(prefix.size(), '\0');
        std::transform(prefix.begin(), prefix.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        if (lowered.find("ecnf") != std::string::npos) {
            return ClauseFormat::ECNF;
        }
        if (lowered.find("cnf") != std::string::npos) {
            return ClauseFormat::CNF;
        }
    }

    return (starCount > plusCount) ? ClauseFormat::ECNF : ClauseFormat::CNF;
}

/**
 * @brief Helper that assigns compact indices to symbolic literal names.
 */
struct SymbolMapper {
    index_t getOrCreate(const std::string& symbol) {
        auto it = ids.find(symbol);
        if (it != ids.end()) {
            return it->second;
        }
        const index_t next = static_cast<index_t>(ids.size() + 1);
        ids.emplace(symbol, next);
        return next;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return ids.size();
    }

   private:
    std::unordered_map<std::string, index_t> ids;
};

/**
 * @brief Lightweight representation describing a parsed literal.
 */
struct LiteralDescriptor {
    index_t index;
    bool complemented;
};

/**
 * @brief Parses a token into a literal descriptor, handling negations and numeric forms.
 * @param token Raw token from the input stream.
 * @param mapper Symbol mapper for non-numeric identifiers.
 * @return LiteralDescriptor containing index and complement state.
 * @throws QSATException When the token is malformed or resolves to index 0.
 */
LiteralDescriptor parseSymbolToken(const std::string& token, SymbolMapper& mapper) {
    std::string working = trimCopy(token);
    if (working.empty()) {
        throw QSATException("Encountered empty literal while parsing expression.");
    }

    bool complemented = false;
    std::size_t pos = 0;
    while (pos < working.size() && (working[pos] == '~' || working[pos] == '!')) {
        complemented = !complemented;
        ++pos;
    }

    working = trimCopy(working.substr(pos));
    if (working.empty()) {
        throw QSATException("Invalid literal token: " + token);
    }

    while (!working.empty() && working.back() == '\'') {
        complemented = !complemented;
        working.pop_back();
    }

    working = trimCopy(working);
    if (working.empty()) {
        throw QSATException("Invalid literal token: " + token);
    }

    bool numeric = true;

    for (std::size_t i = 0; i < working.size(); ++i) {
        if (i == 0 && (working[i] == '+' || working[i] == '-')) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(working[i]))) {
            numeric = false;
            break;
        }
    }

    if (numeric) {
        if (working.size() == 1 && (working[0] == '+' || working[0] == '-')) {
            throw QSATException("Invalid numeric literal: " + token);
        }

        long long value = 0;
        try {
            value = std::stoll(working);
        } catch (const std::exception&) {
            throw QSATException("Invalid numeric literal: " + token);
        }

        if (value == 0) {
            throw QSATException("Literal index cannot be zero.");
        }

        const bool isNegative = value < 0;
        const index_t index = static_cast<index_t>(std::llabs(value));
        const bool finalComplemented = complemented != isNegative;
        return {index, finalComplemented};
    }

    return {mapper.getOrCreate(working), complemented};
}

/**
 * @brief Converts a LiteralDescriptor into a literal expression variant.
 * @param descriptor Parsed literal description.
 * @return Expression containing a newly constructed Literal.
 */
Expression makeLiteralExpression(const LiteralDescriptor& descriptor) {
    return Expression(LiteralPtr(new Literal(descriptor.index, descriptor.complemented)));
}

/**
 * @brief Builds a monomial expression from parsed literal descriptors.
 * @param literals Collection of literal descriptors.
 * @return Expression containing a new Monomial instance.
 * @throws QSATException If no literals are provided.
 */
Expression makeMonomialExpression(const std::vector<LiteralDescriptor>& literals) {
    if (literals.empty()) {
        throw QSATException("Monomial must contain at least one literal.");
    }

    LiteralSet literalSet;
    literalSet.reserve(literals.size());

    for (const auto& literal : literals) {
        literalSet.emplace_back(std::make_unique<Literal>(literal.index, literal.complemented));
    }

    return Expression(MonomialPtr(new Monomial(literalSet)));
}

/**
 * @brief Counts the number of unit clauses within the parsed clause set.
 * @param clauses Parsed clauses.
 * @return Number of clauses that contain a single expression.
 */
count_t countUnitClauses(const ClauseSet& clauses) {
    count_t units = 0;
    for (const auto& clause : clauses) {
        if (clause != nullptr && clause->size() == 1) {
            ++units;
        }
    }
    return units;
}

}  // namespace

/**
 * @brief Parses the formula from a file path.
 * @param filename Path to the DIMACS CNF or ECNF file.
 * @throws QSATException if the file cannot be opened or parsing fails.
 */
void BooleanFormula::parseFrom(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw QSATException("Cannot open formula file: " + filename);
    }
    parseFrom(file);
}

/**
 * @brief Parses the formula from a string buffer.
 * @param content String containing the Boolean formula in DIMACS format.
 * @throws QSATException if the string content is malformed.
 */
void BooleanFormula::parseFromString(const std::string& content) {
    std::istringstream iss(content);
    parseFrom(iss);
}

/**
 * @brief Parses a Boolean formula in DIMACS CNF or ECNF format.
 * @details
 * This parser supports the standard DIMACS CNF format as well as extended comments
 * describing input signal mappings (e.g., "c IN1[0] -> 50"). The parser initializes
 * the formula parameters and constructs a set of clauses from the literal lists.
 *
 * Example Input:
 * @code
 * p cnf 158 362
 * c IN1[0] -> 50
 * -2 1 0
 * 3 -4 0
 * @endcode
 *
 * Lines beginning with:
 * - `c` are treated as comments or mappings (ignored or stored optionally),
 * - `p` defines the problem type (CNF or ECNF) and counts: number of variables, and the number of clauses,
 * - other lines are treated as clause definitions ending with `0`.
 *
 * @param in Input stream containing the Boolean formula.
 * @throws QSATException if the file format is invalid or malformed.
 */
void BooleanFormula::parseFrom(std::istream& in) {
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string trimmed = trimCopy(content);

    clauses.clear();
    parameters.inputCount = 0;
    parameters.clauseCount = 0;
    parameters.unitCount = 0;
    parameters.clauseType = ClauseFormat::CNF;

    if (trimmed.empty()) {
        throw QSATException("Formula input is empty.");
    }

    if (isDimacsLike(trimmed)) {
        const bool ecnfDoubleZeroStyle = detectEcnfDoubleZeroStyle(trimmed);
        std::istringstream dimacsStream(trimmed);
        std::string line;
        bool headerFound = false;
        ClauseFormat format = ClauseFormat::CNF;
        count_t headerVarCount = 0;
        count_t headerClauseCount = 0;
        count_t actualMaxVariable = 0;

        std::unique_ptr<Clause> currentClause = nullptr;
        std::vector<int> currentMonomial;
        bool ecnfClauseParity = false;
        bool clauseUsesExplicitSeparator = false;

        std::size_t lineNumber = 0;
        bool lastTokenWasZero = false;

        auto ensureClause = [&](ClauseFormat clauseFormat) {
            if (!currentClause) {
                currentClause = std::make_unique<Clause>(ExpressionSet(), clauseFormat);
            }
        };

        auto addMonomialFromInts = [&](const std::vector<int>& literalValues) {
            if (literalValues.empty()) {
                return;
            }

            std::vector<LiteralDescriptor> descriptors;
            descriptors.reserve(literalValues.size());

            for (const int literalValue : literalValues) {
                if (literalValue == 0) {
                    throw QSATException("Unexpected 0 inside monomial while parsing ECNF DIMACS input.");
                }

                const bool complemented = literalValue < 0;
                const index_t index = static_cast<index_t>(std::abs(literalValue));
                if (index == 0) {
                    throw QSATException("Literal index cannot be zero.");
                }
                descriptors.push_back({index, complemented});
                actualMaxVariable = std::max(actualMaxVariable, static_cast<count_t>(index));
            }

            ensureClause(ClauseFormat::ECNF);
            currentClause->addExpression(makeMonomialExpression(descriptors));
        };

        auto flushCurrentMonomial = [&](bool splitIntoSingletons) {
            if (currentMonomial.empty()) {
                return;
            }

            if (splitIntoSingletons) {
                for (const int literalValue : currentMonomial) {
                    addMonomialFromInts(std::vector<int>{literalValue});
                }
            } else {
                addMonomialFromInts(currentMonomial);
            }
            currentMonomial.clear();
        };

        auto finalizeClause = [&]() {
            if (!currentClause || currentClause->empty()) {
                throw QSATException("Encountered empty clause while parsing DIMACS input.");
            }
            if (format == ClauseFormat::ECNF && ecnfClauseParity) {
                currentClause->flipComplemented();
            }
            clauses.push_back(std::move(currentClause));
            currentClause.reset();
            ecnfClauseParity = false;
            clauseUsesExplicitSeparator = false;
        };

        while (std::getline(dimacsStream, line)) {
            ++lineNumber;
            const std::string sanitized = trimCopy(line);

            if (sanitized.empty()) {
                continue;
            }

            const char leading = static_cast<char>(std::tolower(sanitized.front()));
            if (leading == 'c' || leading == '#') {
                continue;
            }

            if (leading == 'p') {
                std::istringstream headerStream(sanitized);
                std::string label;
                std::string fmt;
                if (!(headerStream >> label >> fmt >> headerVarCount >> headerClauseCount)) {
                    throw QSATException("Invalid problem line at line " + std::to_string(lineNumber));
                }

                for (auto& c : fmt) {
                    c = static_cast<char>(std::tolower(c));
                }

                if (fmt == "ecnf" || fmt == "qcnf") {
                    format = ClauseFormat::ECNF;
                } else if (fmt == "cnf") {
                    format = ClauseFormat::CNF;
                } else {
                    throw QSATException("Unsupported DIMACS format: " + fmt);
                }

                headerFound = true;
                currentClause = nullptr;
                currentMonomial.clear();
                ecnfClauseParity = false;
                clauseUsesExplicitSeparator = false;
                lastTokenWasZero = false;
                continue;
            }

            if (!headerFound) {
                throw QSATException("Missing \"p\" problem line before clause data.");
            }

            const bool lineHasExplicitSeparator =
                sanitized.find('*') != std::string::npos || sanitized.find('@') != std::string::npos;
            clauseUsesExplicitSeparator = clauseUsesExplicitSeparator || lineHasExplicitSeparator;

            std::istringstream clauseStream(sanitized);
            std::string token;
            while (clauseStream >> token) {
                if (format == ClauseFormat::CNF) {
                    int value = 0;
                    try {
                        value = std::stoi(token);
                    } catch (const std::exception&) {
                        throw QSATException("Invalid CNF literal token: " + token);
                    }

                    if (value == 0) {
                        if (!currentClause) {
                            throw QSATException("Empty clause encountered in DIMACS CNF input.");
                        }

                        finalizeClause();
                        lastTokenWasZero = true;
                        continue;
                    }

                    if (!currentClause) {
                        currentClause = std::make_unique<Clause>(ExpressionSet(), ClauseFormat::CNF);
                    }

                    const bool complemented = value < 0;
                    const index_t index = static_cast<index_t>(std::abs(value));
                    actualMaxVariable = std::max(actualMaxVariable, static_cast<count_t>(index));
                    currentClause->addExpression(makeLiteralExpression({index, complemented}));
                    lastTokenWasZero = false;
                } else {
                    // ECNF with support for '*' or '@' separators, constant '!' monomials, and 0 (single- or double-zero).
                    if (token == "*" || token == "@") {
                        clauseUsesExplicitSeparator = true;
                        if (currentMonomial.empty() && (!currentClause || (currentClause->empty() && !ecnfClauseParity))) {
                            throw QSATException("Encountered '*' with no literals in current ECNF monomial.");
                        }
                        flushCurrentMonomial(false);
                        lastTokenWasZero = false;
                        continue;
                    }

                    if (token == "!") {
                        const bool splitSingletons = !ecnfDoubleZeroStyle && !clauseUsesExplicitSeparator;
                        flushCurrentMonomial(splitSingletons);
                        ensureClause(ClauseFormat::ECNF);
                        ecnfClauseParity = !ecnfClauseParity;
                        lastTokenWasZero = false;
                        continue;
                    }

                    if (token == "0") {
                        if (ecnfDoubleZeroStyle) {
                            flushCurrentMonomial(false);
                            if (currentClause) {
                                if (lastTokenWasZero) {
                                    finalizeClause();
                                    lastTokenWasZero = false;
                                } else {
                                    lastTokenWasZero = true;
                                }
                            } else {
                                lastTokenWasZero = true;
                            }
                        } else {
                            flushCurrentMonomial(!clauseUsesExplicitSeparator);
                            if (!currentClause || currentClause->empty()) {
                                throw QSATException("Empty clause encountered in DIMACS ECNF input.");
                            }
                            finalizeClause();
                            lastTokenWasZero = true;
                        }
                        continue;
                    }

                    int value = 0;
                    try {
                        value = std::stoi(token);
                    } catch (const std::exception&) {
                        throw QSATException("Invalid ECNF literal token: " + token);
                    }

                    const index_t index = static_cast<index_t>(std::abs(value));
                    if (index == 0) {
                        throw QSATException("Literal index cannot be zero.");
                    }

                    ensureClause(ClauseFormat::ECNF);
                    actualMaxVariable = std::max(actualMaxVariable, static_cast<count_t>(index));
                    currentMonomial.push_back(value);
                    lastTokenWasZero = false;
                }
            }
        }

        if (format == ClauseFormat::CNF) {
            if (currentClause && !currentClause->empty()) {
                clauses.push_back(std::move(currentClause));
            }
        } else {
            flushCurrentMonomial(ecnfDoubleZeroStyle ? false : !clauseUsesExplicitSeparator);
            if (currentClause && !currentClause->empty()) {
                finalizeClause();
            }
        }

        if (!headerFound) {
            throw QSATException("No problem definition line found in DIMACS input.");
        }

        const count_t actualClauseCount = static_cast<count_t>(clauses.size());
        if (headerVarCount != 0 && actualMaxVariable > headerVarCount) {
            throw QSATException("Header declares " + std::to_string(headerVarCount) +
                                " variables but literal index " + std::to_string(actualMaxVariable) + " encountered.");
        }
        if (headerClauseCount != 0 && headerClauseCount != actualClauseCount) {
            throw QSATException("Header clause count " + std::to_string(headerClauseCount) +
                                " does not match parsed count " + std::to_string(actualClauseCount) + ".");
        }

        parameters.clauseType = format;
        parameters.inputCount = headerVarCount != 0 ? headerVarCount : actualMaxVariable;
        parameters.clauseCount = headerClauseCount != 0 ? headerClauseCount : actualClauseCount;
    } else {
        std::istringstream rawStream(trimmed);
        std::string rawLine;
        std::ostringstream expressionBuffer;
        std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> aliasTokenPairs;

        while (std::getline(rawStream, rawLine)) {
            std::string sanitized = trimCopy(rawLine);
            if (sanitized.empty()) {
                continue;
            }

            const char lead = static_cast<char>(std::tolower(sanitized.front()));
            if (lead == 'c' || lead == '#') {
                continue;
            }

            const std::size_t arrowPos = sanitized.find("->");
            const bool hasParenthesis = sanitized.find('(') != std::string::npos || sanitized.find(')') != std::string::npos;
            if (arrowPos != std::string::npos && !hasParenthesis) {
                const std::string lhs = trimCopy(sanitized.substr(0, arrowPos));
                const std::string rhs = trimCopy(sanitized.substr(arrowPos + 2));
                if (lhs.empty() || rhs.empty()) {
                    throw QSATException("Invalid alias definition: " + sanitized);
                }

                aliasTokenPairs.emplace_back(splitByWhitespace(lhs), splitByWhitespace(rhs));
                continue;
            }

            expressionBuffer << sanitized << '\n';
        }

        const std::string expressionBody = trimCopy(expressionBuffer.str());

        std::vector<std::string> clauseStrings;
        if (!expressionBody.empty()) {
            clauseStrings = extractClauseBodies(expressionBody);
        }

        if (clauseStrings.empty() && aliasTokenPairs.empty()) {
            throw QSATException("Unable to locate clause expressions within input string.");
        }

        ClauseFormat format = ClauseFormat::CNF;
        if (!clauseStrings.empty()) {
            format = detectClauseFormat(clauseStrings, expressionBody);
        }

        SymbolMapper mapper;
        index_t maxLiteralIndex = 0;

        auto recordMaxIndex = [&](const LiteralDescriptor& descriptor) {
            maxLiteralIndex = std::max(maxLiteralIndex, descriptor.index);
        };

        for (const auto& clauseText : clauseStrings) {
            if (format == ClauseFormat::CNF) {
                auto clausePtr = std::make_unique<Clause>(ExpressionSet(), ClauseFormat::CNF);

                std::vector<std::string> literalTokens;
                if (clauseText.find('+') != std::string::npos) {
                    literalTokens = splitByDelimiter(clauseText, '+');
                } else {
                    literalTokens = splitByOrKeyword(clauseText);
                    if (literalTokens.empty()) {
                        literalTokens = splitByWhitespace(clauseText);
                    }
                }

                if (literalTokens.empty()) {
                    throw QSATException("CNF clause without literals detected.");
                }

                for (const auto& rawToken : literalTokens) {
                    const auto descriptor = parseSymbolToken(trimCopy(rawToken), mapper);
                    recordMaxIndex(descriptor);
                    clausePtr->addExpression(makeLiteralExpression(descriptor));
                }

                clauses.push_back(std::move(clausePtr));
            } else {
                auto clausePtr = std::make_unique<Clause>(ExpressionSet(), ClauseFormat::ECNF);
                bool clauseParity = false;

                std::vector<std::string> monomialStrings;
                if (clauseText.find('*') != std::string::npos || clauseText.find('@') != std::string::npos) {
                    monomialStrings = splitByAnyDelimiter(clauseText, "*@");
                } else {
                    monomialStrings = splitByXorKeyword(clauseText);
                    if (monomialStrings.empty()) {
                        monomialStrings.push_back(clauseText);
                    }
                }

                for (auto& monomialText : monomialStrings) {
                    std::vector<std::string> tokenList;
                    if (monomialText.find(' ') != std::string::npos) {
                        tokenList = splitByAndKeyword(monomialText);
                        if (tokenList.empty()) {
                            tokenList = splitByWhitespace(monomialText);
                        }
                    } else {
                        tokenList = splitByWhitespace(monomialText);
                    }
                    if (tokenList.empty()) {
                        throw QSATException("ECNF monomial without literals detected.");
                    }

                    std::vector<std::string> filteredTokens;
                    filteredTokens.reserve(tokenList.size());
                    bool constantOnlyMonomial = true;
                    for (auto& token : tokenList) {
                        const std::string trimmedToken = trimCopy(token);
                        if (trimmedToken == "!") {
                            continue;
                        }
                        constantOnlyMonomial = false;
                        filteredTokens.push_back(trimmedToken);
                    }

                    if (constantOnlyMonomial) {
                        clauseParity = !clauseParity;
                        continue;
                    }

                    std::vector<LiteralDescriptor> descriptors;
                    descriptors.reserve(filteredTokens.size());
                    for (auto& token : filteredTokens) {
                        descriptors.push_back(parseSymbolToken(token, mapper));
                    }

                    for (const auto& descriptor : descriptors) {
                        recordMaxIndex(descriptor);
                    }

                    clausePtr->addExpression(makeMonomialExpression(descriptors));
                }

                if (clausePtr->empty()) {
                    throw QSATException("Encountered empty ECNF clause.");
                }

                if (clauseParity) {
                    clausePtr->flipComplemented();
                }

                clauses.push_back(std::move(clausePtr));
            }
        }

        if (!aliasTokenPairs.empty()) {
            format = ClauseFormat::ECNF;

            for (const auto& aliasTokens : aliasTokenPairs) {
                const auto& lhsTokens = aliasTokens.first;
                const auto& rhsTokens = aliasTokens.second;

                if (lhsTokens.empty() || rhsTokens.empty()) {
                    throw QSATException("Invalid alias definition.");
                }

                auto clausePtr = std::make_unique<Clause>(ExpressionSet(), ClauseFormat::ECNF);

                auto parseTokenList = [&](const std::vector<std::string>& tokens) {
                    std::vector<LiteralDescriptor> descriptors;
                    descriptors.reserve(tokens.size());
                    for (const auto& token : tokens) {
                        descriptors.push_back(parseSymbolToken(token, mapper));
                    }
                    for (const auto& descriptor : descriptors) {
                        recordMaxIndex(descriptor);
                    }
                    return descriptors;
                };

                std::vector<LiteralDescriptor> lhsDescriptors = parseTokenList(lhsTokens);
                for (auto& descriptor : lhsDescriptors) {
                    descriptor.complemented = !descriptor.complemented;
                }
                clausePtr->addExpression(makeMonomialExpression(lhsDescriptors));

                std::vector<LiteralDescriptor> rhsDescriptors = parseTokenList(rhsTokens);
                clausePtr->addExpression(makeMonomialExpression(rhsDescriptors));

                clauses.push_back(std::move(clausePtr));
            }
        }

        parameters.clauseType = format;
        parameters.inputCount = static_cast<count_t>(
            std::max<index_t>(maxLiteralIndex, static_cast<index_t>(mapper.size())));
        parameters.clauseCount = static_cast<count_t>(clauses.size());
    }

    parameters.unitCount = countUnitClauses(clauses);
}

};  // namespace qSAT
