import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.simulator_tableau import TablesideSimulator


class MockCompiledOpHandler(CompiledOpHandler[TablesideSimulator]):
    def claim_ops(self) -> set[str]:
        return {"MOCK"}

    def handle_op(self, op: stim.CircuitInstruction, sss: "TablesideSimulator") -> None:
        if op in self.claim_ops():
            raise NotImplementedError(f"mock handle_op called and somehow matched")
        else:
            sss._do_bare_instruction(op)
            return

    def clear(self) -> None:
        raise NotImplementedError(f"mock clear called")


class TestFlipsideSimulator:

    batch_size = 128
    coh = MockCompiledOpHandler()

    def make_simulator(self, circuit):
        return TablesideSimulator(
            circuit=circuit, compiled_op_handler=self.coh, batch_size=self.batch_size
        )

    def test_coords_tracking(self):
        """test qubit_coords, qubit_tags, coords_shifts."""
        circuit = stim.Circuit(
            """
        QUBIT_COORDS(0,0) 0
        QUBIT_COORDS(1,0) 1
        SHIFT_COORDS(5, 5)
        QUBIT_COORDS(0) 5
        QUBIT_COORDS(1, 0) 6
        SHIFT_COORDS(5)
        SHIFT_COORDS(5,5,5)
        QUBIT_COORDS[QUBIT_TAG](0,0) 10
        """
        )
        tss = self.make_simulator(circuit)
        tss.interactive_do(circuit[0])  # QUBIT_COORDS(0,0) 0
        tss.interactive_do(circuit[1])  # QUBIT_COORDS(1,0) 1
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        tss.interactive_do(circuit[2])  # SHIFT_COORDS(5, 5)
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        assert tss.coords_shifts == [5, 5]
        tss.interactive_do(circuit[3])  # QUBIT_COORDS(0) 5
        tss.interactive_do(circuit[4])  # QUBIT_COORDS(1, 0) 6
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5]}
        tss.interactive_do(circuit[5])  # SHIFT_COORDS(5)
        assert tss.coords_shifts == [10, 5]
        tss.interactive_do(circuit[6])  # SHIFT_COORDS(5,5,5)
        assert tss.coords_shifts == [15, 10, 5]
        tss.interactive_do(circuit[7])  # QUBIT_COORDS[QUBIT_TAG](0, 0)
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5], 10: [15, 10]}

    def test_run_and_clear(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
        """
        )
        tss = self.make_simulator(circuit)

        tss.run()

        with pytest.raises(ValueError, match=r"prepare the simulator for a new run"):
            tss.run()

        with pytest.raises(NotImplementedError, match="mock clear called"):
            tss.clear()

        assert tss.qubit_coords == {}
        assert tss.qubit_tags == {}
        assert tss.coords_shifts == []

    def test_flip_simulator_lookthroughs(self):
        circuit = stim.Circuit(
            """
            M 0 1 2 3 4 5 6 7
            DETECTOR(0) rec[-8]
            DETECTOR(1) rec[-7]
            DETECTOR(2) rec[-6]
            DETECTOR(3) rec[-5]
            OBSERVABLE_INCLUDE(0) rec[-4] rec[-3] rec[-2] rec[-1]
        """
        )
        circuit_num_qubits = 8
        circuit_num_measurements = 8
        circuit_num_detectors = 4
        circuit_num_observables = 1

        tss = self.make_simulator(circuit)
        tss.run()

        assert tss.batch_size == self.batch_size
        assert tss.num_qubits == circuit_num_qubits

        assert tss.get_detector_flips().shape == (self.batch_size, circuit_num_detectors)
        assert tss.get_final_measurement_records().shape == (self.batch_size, circuit_num_measurements)
        assert tss.get_observable_flips().shape == (self.batch_size, circuit_num_observables)
