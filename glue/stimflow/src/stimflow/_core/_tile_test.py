from stimflow._core._tile import Tile


def test_basis():
    tile = Tile(bases="XYZX", measure_qubit=0, data_qubits=(1, 2, None, 3))
    assert tile.basis is None

    tile = Tile(bases="XXZX", measure_qubit=0, data_qubits=(1, 2, None, 3))
    assert tile.basis == "X"

    tile = Tile(bases="XXX", measure_qubit=0, data_qubits=(1, 2, 3))
    assert tile.basis == "X"

    tile = Tile(bases="ZZZ", measure_qubit=0, data_qubits=(1, 2, 3))
    assert tile.basis == "Z"

    tile = Tile(bases="ZXZ", measure_qubit=0, data_qubits=(1, 2, 3))
    assert tile.basis is None
