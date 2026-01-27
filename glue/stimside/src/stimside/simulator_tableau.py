import itertools as it
from typing import Literal, Iterable

import numpy as np
import stim  # type: ignore[import-untyped]
from numpy.typing import NDArray

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler


class TablesideSimulator:
    """A convenient correlated error simulator based on stim.TableauSimulator.

    Inherits the advantages and disadvantages of a tableau simulator:
     - Handles all stabilizer operations
     - much slower than a flip simulator (O(N^2) cost, rather than O(1))

    The TablesideSimulator has three major additions over the stim.TableauSimulator:
    1. Extra methods to help use a TableauSimulator:

    2. Additional information regarding the circuit being simulated, particularly:
        - The qubit coordinates (including the effects of SHIFT_COORDS)
            This lets the simulator infer where qubits are, allowing behaviour (like errors)
            to depend on location. (See CompiledOpHandler for a way of adding such behaviour)

    3. CompiledOpHandler, which let the simulator track a state and implement complex behaviours:
        Lets the simulator hold configurable state, and provides an easy way of registering
        repeatable behaviours that depend on the capabilities provides above.

        We take a CompiledOpHandler rather than a OpHandler that we then compile to try
        to simplify the class and to encourage users not to do any slow work inside the simulator.
        You are of course free to implement a CompiledOpHandler by hand rather than by making a
        OpHandler, especially when you're using the simulator outside `sinter.collect`

        We take a CompiledOpHandler at all in order to help guide the dividing line between the
        simulator itself and the custom behaviour the user wants. Two rules of thumb are:
         - everything that is hardware or noise model dependent is in the CompiledOpHandler
    """

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        compiled_op_handler: CompiledOpHandler["TablesideSimulator"],
        seed: int | None = None,
        batch_size: int = 1,
        running_tableau: bool = True,
        construct_reference_circuit: bool = False,
    ) -> None:

        self.circuit = circuit
        self.num_qubits = circuit.num_qubits

        self.seed = seed
        self.batch_size = batch_size

        self.qubit_coords: dict[int, list[float]] = {}
        # qubit_coords is a mapping of stim index --> stim args to the QUBIT_COORD instruction

        self.qubit_tags: dict[int, str] = {}
        # qubit_tags is a mapping of  stim index --> tag string for the QUBIT_COORD instruction

        self.coords_shifts: list[float] = []
        # list of ints to track total shifts from SHIFT_COORD instructions

        self.np_rng = np.random.default_rng(seed=seed)

        self._tableau_simulator = stim.TableauSimulator()
        self._new_circuit = stim.Circuit()
        self._construct_reference_circuit = construct_reference_circuit
        if self._construct_reference_circuit:
            self._new_reference_circuit = stim.Circuit()

        self._running_tableau = running_tableau
        self._finished_running_circuit = False
        # # this exists so that op_handlers can access random numbers other than
        # # via the Bernoulli functions that stim provides
        # # there's a decent argument that they should make the rng themselves if they want one
        # # TODO: reflect upon this

        ## This is used to convert tableau measurement records to detection events
        ## Notice this is a converter for the input circuit, not new_circuit
        self._compiled_m2d_converter = circuit.compile_m2d_converter()

        self._compiled_op_handler = compiled_op_handler

        self._circuit_time = 0
        self._used_interactively = False

        self._final_measurement_records: NDArray[np.bool_] | None = None
        self._detector_flips: NDArray[np.bool_] | None = None
        self._observable_flips: NDArray[np.bool_] | None = None

    ############################################################################
    # Public methods without running interactively
    ############################################################################

    def run(self):
        """run the simulator."""
        if self._used_interactively or self._circuit_time != 0:
            raise ValueError(
                "TablesideSimulator is not designed for interactive use. "
                "Use .clear() to prepare the simulator for a new run. "
            )
        self._do(self.circuit)
        self._finished_running_circuit = True

        if self.batch_size > 1:
            self._final_measurement_records = None
        else:
            if self._running_tableau:
                self._final_measurement_records = np.array(
                    [self.current_tableau_measurement_record()]
                )
            else:
                self._final_measurement_records = None

    def clear(self):
        """clear the simulator so it can be reused.

        This is performance critical, as it's going to be used by the sampler before every batch.
        Avoid just recreating the simulator, we shouldn't have to redo any configuration steps here
        """
        # clear qubit coords, as these can change during the circuit

        # self._tableau_simulator.set_inverse_tableau(stim.Tableau(0))
        # Without a clear method from the TableauSimulator, we will just initiate a new one for now
        self._tableau_simulator = stim.TableauSimulator()
        self._new_circuit.clear()
        self._final_measurement_records = None

        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self._compiled_op_handler.clear()

        self._circuit_time = 0
        self._used_interactively = False

    def get_current_circuit_time(self) -> int:
        return self._circuit_time

    ############################################################################
    # Public interactive methods to run the simulation
    ############################################################################
    def interactive_do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock, /
    ):
        if self._finished_running_circuit:
            raise RuntimeError(
                "Cannot do more operations after finishing an interactive run."
            )
        self._used_interactively = True
        self._do(this)

    def finish_interactive_run(self):
        """Finish an interactive run of the simulator."""
        self._finished_running_circuit = True

    ############################################################################
    # private methods to run the simulation
    ############################################################################

    def _do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock
    ):
        if isinstance(this, stim.Circuit):
            # TODO: improve this, looping in python is slow
            for op in this:
                self._do(op)
        elif isinstance(this, stim.CircuitRepeatBlock):
            # TODO: improve this, looping in python is slow
            loop_body = this.body_copy()
            for _ in range(this.repeat_count):
                self._do(loop_body)
        elif isinstance(this, stim.CircuitInstruction):
            self._do_instruction(this)

    def _do_instruction(self, op: stim.CircuitInstruction):
        """for each instruction, update the state of the simulator."""

        # first handle simulator specific stuff
        if op.name == "QUBIT_COORDS":
            [gt] = op.targets_copy()  # unpack the single target
            qubit_idx = gt.qubit_value

            self.qubit_coords[qubit_idx] = [
                c + s
                for c, s in zip(
                    op.gate_args_copy(), it.chain(self.coords_shifts, it.repeat(0))
                )
            ]

            self.qubit_tags[qubit_idx] = op.tag
            self._new_circuit.append(op)
            return

        elif op.name == "SHIFT_COORDS":
            shifts = op.gate_args_copy()
            for i, s in enumerate(shifts):
                if i >= len(self.coords_shifts):
                    self.coords_shifts.append(s)
                else:
                    self.coords_shifts[i] += s
            self._new_circuit.append(op)
            return

        self._compiled_op_handler.handle_op(op=op, sss=self)
        self._circuit_time += 1

    def _do_bare_instruction(self, op: stim.Circuit | stim.CircuitInstruction):
        """
        To be used by op_handler to apply a stabilizer / error instruction directly
        to the TableauSimulator. Append to the new reference circuit if being constructed.
        """
        self._new_circuit.append(op)
        if self._construct_reference_circuit:
            self._new_reference_circuit.append(op)
        if self._running_tableau:
            self._tableau_simulator.do(op)

    def _do_bare_only_on_tableau(self, op: stim.Circuit | stim.CircuitInstruction):
        """
        To be used by op_handler to apply a stabilizer / error instruction directly
        to the TableauSimulator
        """
        self._new_circuit.append(op)
        if self._running_tableau:
            self._tableau_simulator.do(op)

    def _append_to_new_reference_circuit(
        self, op: stim.Circuit | stim.CircuitInstruction
    ):
        """
        Append to the new reference circuit
        """
        if self._construct_reference_circuit:
            self._new_reference_circuit.append(op)

    def _run_tableau(self):
        """
        Run the tableau if not already running
        """
        if self._running_tableau:
            return
        else:
            self._tableau_simulator.do_circuit(self._new_circuit)
            self._running_tableau = True
    ############################################################################
    # known states methods:
    ############################################################################

    def get_current_noisy_tableau_pauli_state(
        self, targets: Iterable[int], pauli: Literal["X", "Y", "Z"]
    ) -> list[int]:
        """returns true if all targets are in a known state of the given pauli."""
        match pauli:
            case "X":
                peek_pauli = self.peek_x
            case "Y":
                peek_pauli = self.peek_y
            case "Z":
                peek_pauli = self.peek_z
            case _:
                raise ValueError("Unrecognised Pauli")

        pauli_states = []
        for target in targets:
            pauli_states.append(peek_pauli(target))
        return pauli_states
    
    ############################################################################
    # properties
    ############################################################################
    @property
    def num_measurements(self) -> int:
        return self.circuit.num_measurements

    @property
    def num_detectors(self) -> int:
        return self.circuit.num_detectors

    @property
    def num_observables(self) -> int:
        return self.circuit.num_observables

    ############################################################################
    # tableau simulator lookthrough methods
    ############################################################################
    def num_qubits_in_new_circuit(self) -> int:
        return self._new_circuit.num_qubits
    
    def peek_z(self, target: int) -> int:
        self._run_tableau()
        return self._tableau_simulator.peek_z(target)

    def peek_x(self, target: int) -> int:
        self._run_tableau()
        return self._tableau_simulator.peek_x(target)

    def peek_y(self, target: int) -> int:
        self._run_tableau()
        return self._tableau_simulator.peek_y(target)

    def peek_observable_expectation(
        self,
        observable: stim.PauliString,
    ) -> int:
        self._run_tableau()
        return self._tableau_simulator.peek_observable_expectation(observable)

    def current_tableau_measurement_record(self) -> NDArray[np.bool_]:
        if self.batch_size > 1:
            print( "Warning when calling current_tableau_measurement_record: "
                "The tableau simulator is not directly called if batch_size > 1."
            )
        self._run_tableau()
        return np.array(self._tableau_simulator.current_measurement_record())

    def get_final_measurement_records(self) -> NDArray[np.bool_]:
        if self._finished_running_circuit is False:
            raise RuntimeError("The circuit has not been fully run yet.")
        if self._running_tableau and self.batch_size == 1:
            self._final_measurement_records = np.array([self.current_tableau_measurement_record()])
        else:
            self._final_measurement_records = self._new_circuit.compile_sampler(
                    seed=self.seed
                ).sample(shots=self.batch_size, bit_packed=False)
        return self._final_measurement_records
    
    def _convert_measurements_to_detector_flips(self) -> None:
        if not self._finished_running_circuit:
            raise RuntimeError("Have not finished running the circuit.")
        if self._final_measurement_records is None:
            if self._running_tableau and self.batch_size == 1:
                self._final_measurement_records = self.current_tableau_measurement_record()
            else:
                self._detector_flips, self._observable_flips = (
                    self._new_circuit.compile_detector_sampler(seed=self.seed).sample(
                       shots=self.batch_size, separate_observables=True, bit_packed=False
                    )
                )
                return
        self._detector_flips, self._observable_flips = self._compiled_m2d_converter.convert(
            measurements=self._final_measurement_records,
            separate_observables=True,
        )

    def get_detector_flips(self) -> NDArray[np.bool_]:
        if self._detector_flips is None:
            self._convert_measurements_to_detector_flips()
        return self._detector_flips

    def get_observable_flips(self) -> NDArray[np.bool_]:
        if self._observable_flips is None:
            self._convert_measurements_to_detector_flips()
        return self._observable_flips