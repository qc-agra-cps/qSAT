/*
 * Python bindings for qsat -> OpenQASM conversion.
 * Mirrors the helpers exposed by qcore while reusing the
 * existing BooleanFormula parser and QCore bridge.
 */

#include <pybind11_json/pybind11_json.hpp>
#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <GateType.hpp>
#include <QCircuit.hpp>

#include <filesystem>
#include <sstream>
#include <utility>

#include "BooleanFormula.hpp"
#include "Decomposer.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;
namespace nl = nlohmann;
using qSAT::BooleanFormula;
using qSAT::MiterType;

namespace {

qcore::QCircuit buildCircuitFromFile(const std::string& path, MiterType miterType) {
    BooleanFormula formula;
    formula.parseFrom(path);
    return formula.toQCircuit(miterType);
}

qcore::QCircuit buildCircuitFromExpression(const std::string& expression, MiterType miterType) {
    BooleanFormula formula;
    formula.parseFromString(expression);
    return formula.toQCircuit(miterType);
}

qcore::QCircuit buildCircuitFromQasmFile(const std::string& path) {
    qcore::QCircuit circuit;
    circuit.readQCircuit(path);
    return circuit;
}

qcore::QCircuit buildCircuitFromQasmString(const std::string& qasm) {
    qcore::QCircuit circuit;
    std::istringstream is(qasm);
    circuit.readQASM(is);
    return circuit;
}

qcore::QCircuit decomposeCircuit(const qcore::QCircuit& circuit) {
    return qSAT::decomposeToCliffordT(circuit);
}

nl::json circuitToQasm(qcore::QCircuit& circuit, const std::string* outputPath = nullptr) {
    const auto qasmString = circuit.toString(qcore::FileFormat::OpenQASM);
    if (outputPath != nullptr) {
        const auto parent = std::filesystem::path(*outputPath).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        circuit.writeQCircuit(*outputPath);
    }
    return qasmString;
}

nl::json circuitProperties(qcore::QCircuit& circuit) {
    nl::json properties;
    properties["QuantumRegSize"] = circuit.getQregSize();
    properties["ClassicalRegSize"] = circuit.getCregSize();

    nl::json gateSummary;
    for (const auto& element : circuit.getProperties()) {
        gateSummary[qcore::toString(element.first)] = element.second;
    }
    properties["GateSummary"] = gateSummary;
    properties["CircuitDepth"] = circuit.getDepth();
    return properties;
}

}  // namespace

PYBIND11_MODULE(pyqsat, m) {
    m.doc() = R"pbdoc(
        Python interface for the qSAT Boolean formula parser and QASM bridge.
        Converts DIMACS/ECNF files or expression strings to OpenQASM using qcore.
    )pbdoc";

    py::enum_<MiterType>(m, "MiterType", py::arithmetic())
        .value("U_TYPE", MiterType::U_TYPE)
        .value("V_TYPE", MiterType::V_TYPE)
        .export_values();

    m.def(
        "dimacs_to_qasm_file",
        [](const std::string& path, const std::string& outputPath, MiterType miterType) {
            auto circuit = buildCircuitFromFile(path, miterType);
            circuitToQasm(circuit, &outputPath);
            return py::none();
        },
        py::arg("path"),
        py::arg("output_path"),
        py::arg("miter_type") = MiterType::U_TYPE,
        "Parse a DIMACS/ECNF file and write the OpenQASM circuit to disk.");

    m.def(
        "expr_to_qasm_file",
        [](const std::string& expression, const std::string& outputPath, MiterType miterType) {
            auto circuit = buildCircuitFromExpression(expression, miterType);
            circuitToQasm(circuit, &outputPath);
            return py::none();
        },
        py::arg("expression"),
        py::arg("output_path"),
        py::arg("miter_type") = MiterType::U_TYPE,
        "Parse an in-memory CNF/ECNF expression and write the OpenQASM circuit to disk.");

    m.def(
        "dimacs_properties",
        [](const std::string& path, MiterType miterType) {
            auto circuit = buildCircuitFromFile(path, miterType);
            return circuitProperties(circuit);
        },
        py::arg("path"),
        py::arg("miter_type") = MiterType::U_TYPE,
        "Return circuit properties for a DIMACS/ECNF file without creating a temporary QASM file.");

    m.def(
        "expr_properties",
        [](const std::string& expression, MiterType miterType) {
            auto circuit = buildCircuitFromExpression(expression, miterType);
            return circuitProperties(circuit);
        },
        py::arg("expression"),
        py::arg("miter_type") = MiterType::U_TYPE,
        "Return circuit properties for an in-memory CNF/ECNF expression.");

    m.def(
        "qasm_properties",
        [](const std::string& path) {
            auto circuit = buildCircuitFromQasmFile(path);
            return circuitProperties(circuit);
        },
        py::arg("path"),
        "Return circuit properties for an existing OpenQASM file.");

    m.def(
        "qasm_string_properties",
        [](const std::string& qasm) {
            auto circuit = buildCircuitFromQasmString(qasm);
            return circuitProperties(circuit);
        },
        py::arg("qasm"),
        "Return circuit properties for an in-memory OpenQASM program.");

    m.def(
        "decompose_qasm_file",
        [](const std::string& inputPath, const std::string& outputPath) {
            auto decomposed = decomposeCircuit(buildCircuitFromQasmFile(inputPath));
            const auto parent = std::filesystem::path(outputPath).parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent);
            decomposed.writeQCircuit(outputPath);
            return py::none();
        },
        py::arg("input_path"),
        py::arg("output_path"),
        "Expand CCX/MCX gates in a QASM file using the qcore Clifford+T decomposition and write the result.");

    m.def(
        "decompose_qasm_string",
        [](const std::string& qasm) {
            auto decomposed = decomposeCircuit(buildCircuitFromQasmString(qasm));
            (void)decomposed;
            return py::none();
        },
        py::arg("qasm"),
        "Expand CCX/MCX gates in a QASM string using the qcore Clifford+T decomposition.");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
