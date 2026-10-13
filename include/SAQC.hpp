#pragma once

#include "BooleanFormula.hpp"
#include /* QCore:: */ "QCircuit.hpp"
#include /* QCore:: */ "decompose/Clifford_T.hpp"

namespace qSAT {
namespace SAQC {

struct OrderedBooleanFormula : qSAT::BooleanFormula {
    std::size_t max_clique_size;
    qSAT::BooleanFormula base() { return *this; }
};

OrderedBooleanFormula reorder_clique(qSAT::BooleanFormula const &in, std::size_t max_clique_size, std::size_t max_clause_size);
qSAT::BooleanFormula reduce(qSAT::BooleanFormula const &in, std::size_t max_new_variables);
qcore::QCircuit build_qaoa_ansatz(qcore::DecompositionConfiguration const &config,  OrderedBooleanFormula const &in, std::size_t num_ancilla, std::size_t num_reps);

} // namespace SAQC
} // namespace qSAT
