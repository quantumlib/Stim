from typing import List

import stim

from ._zx_graph_solver import zx_graph_to_external_stabilizers, text_diagram_to_zx_graph, ExternalStabilizer


def test_disconnected():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X    X---out
    """)) == [
        ExternalStabilizer(input=stim.PauliString("Z"), output=stim.PauliString("_")),
        ExternalStabilizer(input=stim.PauliString("_"), output=stim.PauliString("Z")),
    ]
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z---out
             |
             X
    """)) == [
        ExternalStabilizer(input=stim.PauliString("Z"), output=stim.PauliString("_")),
        ExternalStabilizer(input=stim.PauliString("_"), output=stim.PauliString("Z")),
    ]
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z---X---out
             |   |
             *---*
    """)) == [
        ExternalStabilizer(input=stim.PauliString("X"), output=stim.PauliString("_")),
        ExternalStabilizer(input=stim.PauliString("_"), output=stim.PauliString("Z")),
    ]


def test_cnot():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X---out
             |
        in---Z---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("CNOT 1 0"))

    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z---out
             |
        in---X---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("CNOT 0 1"))


def test_cz():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z---out
             |
             H
             |
        in---Z---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("CZ 0 1"))


def test_s():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z(pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("S 0"))


def test_s_dag():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z(-pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("S_DAG 0"))


def test_sqrt_x():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X(pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("SQRT_X 0"))


def test_sqrt_x_sqrt_x():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X(pi/2)---X(pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("X 0"))


def test_sqrt_z_sqrt_z():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z(pi/2)---Z(pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("Z 0"))


def test_sqrt_x_dag():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X(-pi/2)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("SQRT_X_DAG 0"))


def test_x():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X(pi)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("X 0"))


def test_z():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---Z(pi)---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("Z 0"))


def test_id():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph("""
        in---X---Z---out
    """)) == external_stabilizers_of_circuit(stim.Circuit("I 0"))


def test_s_state_distill():
    assert zx_graph_to_external_stabilizers(text_diagram_to_zx_graph(r"""
                        *                  *---------------Z--------------------Z-------Z(pi/2)
                       / \                 |               |                    |
                *-----*   *------------Z---+---------------+---Z----------------+-------Z(pi/2)
                |                      |   |               |   |                |
                X---X---Z(pi/2)        X---X---Z(pi/2)     X---X---Z(pi/2)      X---X---Z(pi/2)
                |   |                  |                   |                    |   |
                *---+------------------Z-------------------+--------------------+---Z---Z(pi/2)
                    |                                      |                    |
           in-------Z--------------------------------------Z-------------------Z(pi)--------out
    """)) == external_stabilizers_of_circuit(stim.Circuit("S 0"))


def external_stabilizers_of_circuit(circuit: stim.Circuit) -> List[ExternalStabilizer]:
    n = circuit.num_qubits
    s = stim.TableauSimulator()
    s.do(circuit)
    t = s.current_inverse_tableau()**-1
    stabilizers = []
    for k in range(n):
        p = [0] * n
        p[k] = 1
        stabilizers.append(stim.PauliString(p) + t.x_output(k))
        p[k] = 3
        stabilizers.append(stim.PauliString(p) + t.z_output(k))
    return [ExternalStabilizer.from_dual(e, circuit.num_qubits) for e in stabilizers]
