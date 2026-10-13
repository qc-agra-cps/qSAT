#include "SAQC.hpp"

#include "ParameterSet.hpp"
#include "clipperplus/clipperplus_clique.h"
#include /* QCore:: */ "decompose/Clifford_T.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <set>
#include <stdexcept>
#include <variant>

namespace qSAT::SAQC {

// ===== Helper =====
template <typename T, typename Vec = std::vector<std::unique_ptr<T>>>
Vec make(typename Vec::const_iterator begin, typename Vec::const_iterator end) {
    Vec result;
    std::transform(begin, end, std::back_inserter(result), [](auto const &i) { return std::make_unique<T>(*i); });
    return result;
}
template <typename T, typename Vec = std::vector<std::unique_ptr<T>>> Vec make(Vec const &in) {
    return make<T>(in.begin(), in.end());
}
template <typename T, auto... Args> struct strong_type {
    T value;
    explicit strong_type(T v = {}) : value(v) {}
    T operator=(T v) { return value = v; }
    operator T const &() const { return value; }
};

using ClauseId = strong_type<std::size_t>;

struct EasyClause : std::vector<std::vector<qSAT::Literal>> {
    EasyClause() {}
    EasyClause(qSAT::Clause const &clause) {
        std::vector<std::vector<qSAT::Literal>> result;
        for (auto const &expr : clause.getExpressions()) {
            std::vector<qSAT::Literal> cur;
            if (std::holds_alternative<qSAT::LiteralPtr>(expr)) {
                auto const &lit = std::get<qSAT::LiteralPtr>(expr);
                cur.emplace_back(*lit);
            } else {
                auto const &mon = std::get<qSAT::MonomialPtr>(expr);
                for (auto const &lit : mon->getLiterals()) {
                    cur.emplace_back(*lit);
                }
            }
            this->emplace_back(cur);
        }
    }

    using base = std::vector<std::vector<qSAT::Literal>>;
    using const_iterator = base::const_iterator;

    using base::at;
    using base::begin;
    using base::end;
    using base::operator[];

    const_iterator find_safe_singleton_target() const {
        return std::find_if(begin(), end(), [this](auto const &target) {
            return target.size() == 1 && !std::any_of(begin(), end(), [&target](auto const &other) {
                       return &other != &target && std::any_of(other.begin(), other.end(), [&target](auto const &v) {
                           return v.getIndex() == target[0].getIndex();
                       });
                   });
        });
    }
};

struct EasyCircuit : qcore::QCircuit {
    EasyCircuit(std::size_t num_qubit, std::size_t num_cbit = 0) : qcore::QCircuit(num_qubit, num_cbit) {};

    std::string expr = "";

    void with_control(std::string expr, std::function<void()> fn) {
        auto prev = this->expr;
        this->expr = expr;
        fn();
        this->expr = prev;
    }

    void add_gate(qcore::QGate gate) {
        if (!expr.empty()) {
            gate.setClassical(true);
        }
        addQGate(gate);
    }

    void reset(std::size_t qubit) { add_gate({qcore::GateType::RESET, 1, {qubit}}); }
    void h(std::size_t qubit) { add_gate({qcore::GateType::H, 1, {qubit}}); }
    void x(std::size_t qubit) { add_gate({qcore::GateType::X, 1, {qubit}}); }
    void sdg(std::size_t qubit) { add_gate({qcore::GateType::SDG, 1, {qubit}}); }
    void measure(std::size_t qubit, std::size_t cbit) {
	  add_gate({qcore::GateType::MEASURE, 1, (qcore::CbitSet){static_cast<unsigned int>(cbit)}, (qcore::TargetSet){static_cast<unsigned int>(qubit)}});
    }
    void cx(std::size_t ctrl, std::size_t targ) {
        add_gate({qcore::GateType::CX, 2, (qcore::ControlSet){ctrl}, (qcore::TargetSet){targ}});
    }
    void cz(std::size_t ctrl, std::size_t targ) {
        add_gate({qcore::GateType::CZ, 2, (qcore::ControlSet){ctrl}, (qcore::TargetSet){targ}});
    }
    void csdg(std::size_t ctrl, std::size_t targ) {
        add_gate({qcore::GateType::CSDG, 2, (qcore::ControlSet){ctrl}, (qcore::TargetSet){targ}});
    }
    void mcx(std::vector<std::size_t> ctrl, std::size_t targ) {
        // add_gate({qcore::GateType::MCX, ctrl.size() + 1, (qcore::ControlSet)ctrl, (qcore::TargetSet){targ}});
		std::size_t dirty = 0;
		while (dirty == targ || std::find(ctrl.begin(), ctrl.end(), dirty) != ctrl.end()) dirty++;
		auto decompose = qcore::decompose_resolve_Toffoli(qcore::DecompositionConfiguration::ReduceTDepth.withRelativePhase(false, true), ctrl, targ, {}, {dirty});
        for (auto const &gate : decompose.getGates()) {
            add_gate(*gate);
        }
    }
    void rz(std::size_t qubit, std::string theta) {
        qcore::RotationMap map;
        map[qcore::THETA] = theta;
        add_gate({qcore::GateType::RZ, 1, map, {qubit}});
    }
    void rx(std::size_t qubit, std::string theta) {
        qcore::RotationMap map;
        map[qcore::THETA] = theta;
        add_gate({qcore::GateType::RX, 1, map, {qubit}});
    }
    void barrier() { add_gate({qcore::GateType::BARRIER, 0, {}}); }
};

std::vector<ClauseId> find_max_clique(std::vector<qSAT::ClausePtr> const &clauses, std::vector<ClauseId> const &ids) {
    Eigen::MatrixXd adj(ids.size(), ids.size());
    for (std::size_t i = 0; i < ids.size(); ++i) {
        for (std::size_t j = i; j < ids.size(); ++j) {
            auto is_adjacent = (i == j || clauses[ids[i]]->shares_variable_with(*clauses[ids[j]]));
            adj(i, j) = adj(j, i) = is_adjacent ? 0 : 1;
        }
    }

    auto [clique, certificate] = clipperplus::find_clique(adj);
    std::sort(clique.begin(), clique.end());
    std::vector<ClauseId> max_clique;
    for (auto index : clique) {
        max_clique.push_back(ids[index]);
    }
    return max_clique;
}

OrderedBooleanFormula reorder_clique(qSAT::BooleanFormula const &in, std::size_t max_clique_size,
                                    std::size_t max_clause_size) {
    auto const &clauses = in.getClauses();
    std::vector<ClauseId> clause_ids(clauses.size());
    std::iota(clause_ids.begin(), clause_ids.end(), 0);

    std::map<int, int> clause_connections;
    for (std::size_t i = 0; i < clauses.size(); ++i) {
        for (std::size_t j = i + 1; j < clauses.size(); ++j) {
            if (clauses[i]->shares_variable_with(*clauses[j])) {
                clause_connections[i]++;
                clause_connections[j]++;
            }
        }
    }
    std::sort(clause_ids.begin(), clause_ids.end(),
              [&clause_connections](auto i, auto j) { return clause_connections[i] < clause_connections[j]; });

    qSAT::ClauseSet new_clause_order;
    std::set<ClauseId> used_clauses;

    // Schedule large clauses immediately
    for (std::size_t i = 0; i < clauses.size(); ++i) {
        if (clauses[i]->getNumLiterals() > max_clause_size) {
            new_clause_order.emplace_back(std::make_unique<qSAT::Clause>(*clauses[i]));
            used_clauses.emplace(i);
            std::erase(clause_ids, i);
        }
    }

    std::size_t max_clique_found = 0;
    while (!clause_ids.empty()) {
        auto max_new_clauses = std::min(clause_ids.size(), max_clique_size);
        std::vector<ClauseId> least_connected(clause_ids.begin(), clause_ids.begin() + max_new_clauses);
        auto max_clique = find_max_clique(clauses, least_connected);
        max_clique_found = std::max(max_clique_found, max_clique.size());
        std::sort(max_clique.begin(), max_clique.end(),
                  [&](int a, int b) { return clauses[a]->getNumLiterals() > clauses[b]->getNumLiterals(); });

        bool first = true;
        for (auto clause_id : max_clique) {
            new_clause_order.emplace_back(std::make_unique<qSAT::Clause>(*clauses[clause_id]));
            if (first) {
                new_clause_order.back()->getAttributes().clique_start = true;
                first = false;
            }
            used_clauses.emplace(clause_id);
            std::erase(clause_ids, clause_id);
        }
    }
    if (used_clauses.size() != clauses.size()) {
        throw std::logic_error{
            std::format("expected {} expressions, only mapped {}", clauses.size(), used_clauses.size())};
    }

    OrderedBooleanFormula out;
    out.max_clique_size = max_clique_found;
    out.setParameters(qSAT::ParameterSet{in.getParameters()});
    out.setClauses(std::move(new_clause_order));
    return out;
}

qSAT::BooleanFormula reduce(qSAT::BooleanFormula const &in, std::size_t max_new_variables) {
    auto clauses = make<qSAT::Clause>(in.getClauses());
    auto parameters = in.getParameters();

    while (max_new_variables--) {
        bool did_split = false;

        std::size_t max_clause_len = 0;
        for (auto const &clause : clauses) {
            max_clause_len = std::max(max_clause_len, clause->getMonomial()->getLiterals().size());
        }

        for (auto const &clause : clauses) {
            auto const &monomial = clause->getMonomial();
            auto num_literals = monomial->getLiterals().size();
            if (num_literals < 4 || num_literals < max_clause_len)
                continue;

            parameters.inputCount++;
            parameters.clauseCount++;

            std::size_t index = parameters.inputCount;
            auto const &literals = monomial->getLiterals();
            auto split = std::floor((double)num_literals / 2);
            auto a = make<qSAT::Literal>(literals.begin(), literals.begin() + split);
            auto b = make<qSAT::Literal>(literals.begin() + split, literals.end());
            auto find_clause = [&]() { return std::find(clauses.begin(), clauses.end(), clause); };

            if (parameters.clauseType == qSAT::ClauseFormat::CNF ||
                parameters.clauseType == qSAT::ClauseFormat::DIMACS) {
                a.emplace_back(new qSAT::Literal(index, false));
                b.emplace_back(new qSAT::Literal(index, true));

                qSAT::ClausePtr clause_a = std::make_unique<qSAT::Clause>(*clause);
                qSAT::ClausePtr clause_b = std::make_unique<qSAT::Clause>(*clause);
                clause_a->getMonomial()->getLiterals() = std::move(a);
                clause_b->getMonomial()->getLiterals() = std::move(b);
                clauses.erase(find_clause());
                clauses.emplace_back(std::move(clause_a));
                clauses.emplace_back(std::move(clause_b));
            } else {
                a.emplace_back(new qSAT::Literal(index, false));
                qSAT::ClausePtr clause_a = std::make_unique<qSAT::Clause>(*clause);
                clause_a->getMonomial()->getLiterals() = std::move(a);
                qSAT::ExpressionSet expressions;
                expressions.emplace_back(std::make_unique<qSAT::Literal>(index, true));
                expressions.emplace_back(std::make_unique<qSAT::Monomial>(std::move(b)));
                qSAT::ClausePtr clause_b =
                    std::make_unique<qSAT::Clause>(std::move(expressions), qSAT::ClauseFormat::ECNF);
                clauses.erase(find_clause());
                clauses.emplace_back(std::move(clause_a));
                clauses.emplace_back(std::move(clause_b));
            }
            break;
        }
    }

    qSAT::BooleanFormula out;
    out.setClauses(std::move(clauses));
    out.setParameters(std::move(parameters));
    return out;
}

qcore::QCircuit build_qaoa_ansatz(qcore::DecompositionConfiguration const &config, OrderedBooleanFormula const &in, std::size_t num_ancilla, std::size_t num_reps) {
    std::vector<EasyClause> clauses;
    std::transform(in.getClauses().begin(), in.getClauses().end(), std::back_inserter(clauses),
                   [](auto const &c) { return *c; });
    auto needs_xor_workspace = std::any_of(clauses.begin(), clauses.end(),
                                           [](auto const &c) { return c.find_safe_singleton_target() == c.end(); });
    auto recommended_ancilla = in.max_clique_size;
    auto used_ancilla = std::min(recommended_ancilla, num_ancilla);

    std::vector<qcore::Qubit> Q;
    std::vector<qcore::Qubit> A;
    std::vector<qcore::Qubit> AC;
    std::vector<qcore::Qubit> W;

    std::size_t qubit_count = 0;
    std::size_t cbit_count = 0;
    for (std::size_t i = 0; i < in.getParameters().inputCount; ++i) {
        Q.emplace_back(qubit_count++);
    }
    if (needs_xor_workspace) {
        W.emplace_back(qubit_count++);
    }
    for (std::size_t i = 0; i < used_ancilla; ++i) {
        A.emplace_back(qubit_count++);
        AC.emplace_back(cbit_count++);
    }

    std::size_t last_ancilla = 0;
    EasyCircuit circ(qubit_count, cbit_count);

    for (auto q : Q) {
        circ.h(q);
    }
    circ.barrier();

    for (std::size_t rep = 0; rep < num_reps; ++rep) {
        std::size_t i = 0;
        for (auto const &clause : clauses) {
            if (in.getClauses()[i]->getAttributes().clique_start && last_ancilla > A.size()) {
                last_ancilla = 0;
            }

            if (in.getParameters().clauseType != qSAT::ClauseFormat::ECNF) {
                throw std::invalid_argument{"not implemented"};
            }

            EasyClause::const_iterator selected = clause.find_safe_singleton_target();
            auto target_literal = std::distance(clause.begin(), selected);
            std::size_t workspace_qubit = selected != clause.end() ? Q[(*selected)[0].getIndex()-1] : W[0];
            std::optional<std::size_t> ancilla_qubit = {};
            if (!A.empty() && last_ancilla < A.size() && clause.back().size() > 1) {
                selected = clause.end();
                workspace_qubit = A[last_ancilla];
                ancilla_qubit = last_ancilla;
                last_ancilla = (last_ancilla + 1) % recommended_ancilla;
            }

            bool negated_singleton_parity = in.getClauses()[i]->isComplementedClause();
            std::size_t ini_gates = circ.getGates().size();
            std::vector<std::size_t> rel_ctrls;
            enum { None, CCX, C3X } rel_type = None;
            for (auto const &term : std::ranges::reverse_view{clause}) {
                if (term.size() == 1 && term[0].isComplemented()) {
                    negated_singleton_parity = !negated_singleton_parity;
                }

                if (&term == &*selected) {
                    continue;
                }

                if (term.size() == 1) {
                    circ.cx(Q[term[0].getIndex()-1], workspace_qubit);
                } else {
                    std::vector<std::size_t> ctrl;
                    for (auto v : term) {
                        if (v.isComplemented())
                            circ.x(Q[v.getIndex()-1]);
                        ctrl.emplace_back(Q[v.getIndex()-1]);
                    }
                    if (ctrl.size() == 2) {
						auto c = qcore::decompose_RCCX_Clifford_T_depth2(ctrl[0], ctrl[1], workspace_qubit, false);
                        for (auto &g : c.getGates()) {
                            circ.addQGate(*g);
                        }
                        if (ancilla_qubit) {
							circ.sdg(workspace_qubit);
                            rel_type = CCX;
                            ini_gates = circ.getGates().size();
                            rel_ctrls = ctrl;
                        }
                    } else if (ctrl.size() == 3) {
					  auto c = qcore::decompose_RC3X_Clifford_T_depth4(ctrl[0], ctrl[1], ctrl[2], workspace_qubit, false);
                        for (auto &g : c.getGates()) {
                            circ.addQGate(*g);
                        }
                        if (ancilla_qubit) {
                            circ.sdg(workspace_qubit);
                            rel_type = C3X;
                            ini_gates = circ.getGates().size();
                            rel_ctrls = ctrl;
                        }
                    } else {
                        circ.mcx(ctrl, workspace_qubit);
                    }
                }
            }

            std::size_t fini_gates = circ.getGates().size();
            circ.rz(workspace_qubit, std::format("{}*gamma[{}]", !negated_singleton_parity ? "1" : "-1", rep));
            while (fini_gates != ini_gates) {
                fini_gates--;
                circ.addQGate((*circ.getGates()[fini_gates]).inverse());
            }
            switch (rel_type) {
            case CCX:
                circ.h(workspace_qubit);
                circ.measure(workspace_qubit, AC[*ancilla_qubit]);
                circ.reset(workspace_qubit);
                circ.with_control(std::format("c[{}]", AC[*ancilla_qubit]),
                                  [&]() { circ.cz(rel_ctrls[0], rel_ctrls[1]); });

                break;
            case C3X:
                circ.h(workspace_qubit);
                circ.reset(workspace_qubit);
                circ.measure(workspace_qubit, AC[*ancilla_qubit]);
                circ.with_control(std::format("c[{}]", AC[*ancilla_qubit]), [&]() {
                    circ.cx(rel_ctrls[0], rel_ctrls[2]);
                    auto c = qcore::decompose_SRCCX_Clifford_T(rel_ctrls[0], rel_ctrls[1], rel_ctrls[2]);
                    for (auto &g : c.getGates()) {
                        if (g->getType() == qcore::GateType::H)
                            continue; // we want CCiZ
                        circ.add_gate(*g);
                    }
                });
                circ.with_control(std::format("!c[{}]", AC[*ancilla_qubit]), [&]() {
                    circ.csdg(rel_ctrls[0], rel_ctrls[1]); //
                });
                break;
            default:
                break;
            }

            circ.barrier();
            i += 1;
        }
        for (auto q : Q) {
            circ.rx(q, std::format("2*beta[{}]", rep));
        }
        if (rep + 1 < num_reps) {
            circ.barrier();
        }
    }

    return circ;
}

} // namespace qSAT::SAQC
