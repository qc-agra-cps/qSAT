from pyqsat import (
    MiterType, BooleanFormula,

    saqc as _saqc
)
import qcore

def build_qsat_circuit(formula: BooleanFormula,
                       config: qcore.decomposition.configuration,
                       miter_type: MiterType):
    return formula.toQCircuit(config, miter_type)

def build_qaoa_circuit(config: qcore.decomposition.configuration,
                       formula: BooleanFormula,
                       num_reps: int = 1, num_ancilla: int = 1, max_clause_size: int = 4,
                       max_new_variables: int = 1, max_clique_size: int = 10):
    return _saqc.build_qaoa_ansatz(
        config,
        _saqc.reorder_clique(
            _saqc.reduce(formula, max_new_variables),
            max_clique_size,
            max_clause_size,
        ),
        num_ancilla,
        num_reps,
    )
