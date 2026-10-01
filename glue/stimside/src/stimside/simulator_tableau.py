import itertools as it
import weakref
from typing import Literal, Iterable

import numpy as np
import stim  # type: ignore[import-untyped]
from numpy.typing import NDArray

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
)
from stimside.util.unleaked_to_leaked import UnleakedToLeakedRecordsMixin

_STANDARD_LEAKAGE_PARAM_TYPES = (
    LeakageConditioningParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageMeasurementParams,
)


class TablesideSimulator(UnleakedToLeakedRecordsMixin):
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

    @property
    def _tableau_simulator(self) -> stim.TableauSimulator:
        if self._tableau_simulator_inst is None:
            self._tableau_simulator_inst = stim.TableauSimulator(seed=self._tab_seed)
        return self._tableau_simulator_inst

    @_tableau_simulator.setter
    def _tableau_simulator(self, val: stim.TableauSimulator | None) -> None:
        self._tableau_simulator_inst = val

    @property
    def _compiled_m2d_converter(self) -> stim.CompiledMeasurementsToDetectionEventsConverter:
        if self._compiled_m2d_converter_inst is None:
            if self._precompiled_circuit is not None:
                self._compiled_m2d_converter_inst = self._precompiled_circuit.m2d_converter
            else:
                self._compiled_m2d_converter_inst = self.circuit.compile_m2d_converter()
        return self._compiled_m2d_converter_inst  # type: ignore[return-value]

    @_compiled_m2d_converter.setter
    def _compiled_m2d_converter(self, val: object) -> None:
        self._compiled_m2d_converter_inst = val

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        compiled_op_handler: CompiledOpHandler["TablesideSimulator"],
        seed: int | None = None,
        batch_size: int = 1,
        running_tableau: bool = True,
        construct_reference_circuit: bool = False,
        use_cpp_kernels: bool | None = None,
        sync_tableside_rng: bool | None = None,
        record_unleaked_to_leaked: bool = False,
    ) -> None:

        self.circuit = circuit
        self.num_qubits = circuit.num_qubits

        self.seed = seed
        self.batch_size = batch_size
        self.record_unleaked_to_leaked = bool(record_unleaked_to_leaked)
        self._unleaked_to_leaked_events: list[list[int]] = (
            [[] for _ in range(batch_size)]
            if self.record_unleaked_to_leaked
            else []
        )
        self._shot_counter = 0

        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self._sync_tableside_rng = bool(sync_tableside_rng)

        self.use_cpp_kernels = (
            True if use_cpp_kernels is None else bool(use_cpp_kernels)
        ) and not self._sync_tableside_rng

        self.np_rng = np.random.default_rng(seed=seed)
        self._rng = self.np_rng if seed is not None else np.random.default_rng(seed=None)

        self._tab_seed = int(seed) if seed is not None else int(self._rng.integers(0, 2**31))
        self._tableau_simulator_inst: stim.TableauSimulator | None = None
        self._new_circuit = stim.Circuit()
        self._construct_reference_circuit = construct_reference_circuit
        if self._construct_reference_circuit:
            self._new_reference_circuit = stim.Circuit()

        self._compiled_op_handler = compiled_op_handler
        self._precompiled_circuit = getattr(
            compiled_op_handler, "_precompiled_circuit", None
        )
        if (
            self._precompiled_circuit is None
            and hasattr(compiled_op_handler, "ops_to_params")
            and hasattr(compiled_op_handler, "unconditional_condition_on_U")
        ):
            from stimside.util.tableside_kernels import get_precompiled_circuit

            self._precompiled_circuit = get_precompiled_circuit(
                circuit, bool(getattr(compiled_op_handler, "unconditional_condition_on_U"))
            )
            compiled_op_handler._precompiled_circuit = self._precompiled_circuit  # type: ignore[attr-defined]

        self._compiled_m2d_converter_inst: object | None = None
        if self._precompiled_circuit is not None:
            if self._sync_tableside_rng:
                eff_rt = bool(running_tableau)
            else:
                eff_rt = bool(running_tableau and self._precompiled_circuit.requires_tableau)
        else:
            eff_rt = bool(running_tableau)

        self._initial_running_tableau = eff_rt
        self._running_tableau = eff_rt
        if eff_rt:
            self._tableau_simulator_inst = stim.TableauSimulator(seed=self._tab_seed)
        self._finished_running_circuit = False

        self._cpp_runner = None
        if self.use_cpp_kernels and self._precompiled_circuit is not None:
            self._cpp_runner = self._precompiled_circuit.acquire_cpp_runner(
                seed=self._tab_seed
            )
            weakref.finalize(
                self,
                self._precompiled_circuit.release_cpp_runner,
                self._cpp_runner,
            )
            if hasattr(self._compiled_op_handler, "state"):
                np.copyto(self._cpp_runner.state, self._compiled_op_handler.state)
                self._compiled_op_handler.state = self._cpp_runner.state

        self._circuit_time = 0
        self._used_interactively = False

        self._final_measurement_records: NDArray[np.bool_] | None = None
        self._detector_flips: NDArray[np.bool_] | None = None
        self._observable_flips: NDArray[np.bool_] | None = None
        self._asymmetric_readout_postproc: list[tuple[int, float, float, bool]] = []

    ############################################################################
    # Public methods without running interactively
    ############################################################################

    def _run_single_trajectory(self):
        pre = self._precompiled_circuit
        coh = self._compiled_op_handler
        can_use_fast = (
            pre is not None
            and hasattr(coh, "ops_to_params")
            and len(coh.ops_to_params) == pre.num_claimed_ops
            and (
                coh.ops_to_params is pre.parsed_ops
                or all(
                    isinstance(v, _STANDARD_LEAKAGE_PARAM_TYPES)
                    for v in coh.ops_to_params.values()
                )
            )
        )

        if can_use_fast:
            from stimside.util.tableside_kernels import (
                run_tableside_python_v2,
                run_tableside_sync_rng,
            )

            if self._sync_tableside_rng:
                run_tableside_sync_rng(self, pre)
            elif self.use_cpp_kernels and self._cpp_runner is not None:
                self._cpp_runner.run(self)
            else:
                run_tableside_python_v2(self, pre)
        else:
            self._do(self.circuit)

    def run(self):
        """run the simulator."""
        if self._used_interactively or self._circuit_time != 0:
            raise ValueError(
                "TablesideSimulator is not designed for interactive use. "
                "Use .clear() to prepare the simulator for a new run. "
            )

        if self._sync_tableside_rng and self.batch_size > 1:
            orig_batch = self.batch_size
            meas_list = []
            saved_events = [[] for _ in range(orig_batch)] if self.record_unleaked_to_leaked else []
            self.batch_size = 1
            try:
                for b in range(orig_batch):
                    if b > 0:
                        self.clear()
                    self._run_single_trajectory()
                    self._shot_counter += 1
                    rec = self.current_tableau_measurement_record()
                    meas_list.append(rec.copy())
                    if self.record_unleaked_to_leaked:
                        saved_events[b] = list(self._unleaked_to_leaked_events[0])
            finally:
                self.batch_size = orig_batch
            self._finished_running_circuit = True
            self._final_measurement_records = np.vstack(meas_list)
            if self.record_unleaked_to_leaked:
                self._unleaked_to_leaked_events = saved_events
                return self.get_unleaked_to_leaked_records()
            return None

        self._run_single_trajectory()

        self._shot_counter += 1
        self._finished_running_circuit = True

        if self.batch_size > 1:
            self._final_measurement_records = None
            if (
                self.record_unleaked_to_leaked
                and len(self._unleaked_to_leaked_events) == self.batch_size
                and len(self._unleaked_to_leaked_events[0]) > 0
                and len(self._unleaked_to_leaked_events[1]) == 0
            ):
                ev0 = list(self._unleaked_to_leaked_events[0])
                for b in range(1, self.batch_size):
                    self._unleaked_to_leaked_events[b] = list(ev0)
        else:
            if self._running_tableau:
                rec = self.current_tableau_measurement_record()
                self._final_measurement_records = np.array([rec])
            else:
                self._final_measurement_records = None

        if self.record_unleaked_to_leaked:
            return self.get_unleaked_to_leaked_records()
        return None

    def clear(self):
        """clear the simulator so it can be reused.

        This is performance critical, as it's going to be used by the sampler before every batch.
        Avoid just recreating the simulator, we shouldn't have to redo any configuration steps here
        """
        self._running_tableau = self._initial_running_tableau
        if self.seed is not None:
            shot_seed = int(self.seed) + int(self._shot_counter)
            self.np_rng = np.random.default_rng(seed=shot_seed)
            self._rng = self.np_rng
            self._tab_seed = shot_seed
            self._tableau_simulator_inst = (
                stim.TableauSimulator(seed=shot_seed) if self._running_tableau else None
            )
            if self._cpp_runner is not None:
                self._cpp_runner.clear(shot_seed)
        else:
            self._tab_seed = int(self._rng.integers(0, 2**31))
            self._tableau_simulator_inst = (
                stim.TableauSimulator(seed=self._tab_seed)
                if self._running_tableau
                else None
            )
            if self._cpp_runner is not None:
                self._cpp_runner.clear(self._tab_seed)

        self._new_circuit.clear()
        if self._construct_reference_circuit:
            self._new_reference_circuit = stim.Circuit()
        self._final_measurement_records = None
        self._detector_flips = None
        self._observable_flips = None
        self._asymmetric_readout_postproc.clear()
        self._finished_running_circuit = False

        self.qubit_coords = {}
        self.qubit_tags = {}
        self.coords_shifts = []

        self._compiled_op_handler.clear()
        if self._cpp_runner is not None and hasattr(self._compiled_op_handler, "state"):
            self._compiled_op_handler.state = self._cpp_runner.state

        if self.record_unleaked_to_leaked:
            self._unleaked_to_leaked_events = [
                [] for _ in range(self.batch_size)
            ]

        self._circuit_time = 0
        self._used_interactively = False

    def _record_unleaked_to_leaked_count(
        self, count: int, op_idx: int | None = None, shot_idx: int = 0
    ) -> None:
        """Record `count` unleaked-to-leaked transitions at unrolled `op_idx` for `shot_idx`."""
        if not self.record_unleaked_to_leaked or count <= 0:
            return
        if op_idx is None:
            op_idx = self._circuit_time
        idx_val = int(op_idx)
        ev = self._unleaked_to_leaked_events[shot_idx]
        if count == 1:
            ev.append(idx_val)
        else:
            ev.extend([idx_val] * int(count))

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
        if not self._used_interactively:
            self._used_interactively = True
            self._running_tableau = True
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
            for op in this:
                self._do(op)
        elif isinstance(this, stim.CircuitRepeatBlock):
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
            if self._construct_reference_circuit:
                self._new_reference_circuit.append(op)
            self._circuit_time += 1
            return

        elif op.name == "SHIFT_COORDS":
            shifts = op.gate_args_copy()
            for i, s in enumerate(shifts):
                if i >= len(self.coords_shifts):
                    self.coords_shifts.append(s)
                else:
                    self.coords_shifts[i] += s
            self._new_circuit.append(op)
            if self._construct_reference_circuit:
                self._new_reference_circuit.append(op)
            self._circuit_time += 1
            return

        self._compiled_op_handler.handle_op(op=op, sss=self)
        self._circuit_time += 1

    def _do_bare_instruction(self, op: stim.Circuit | stim.CircuitInstruction):
        """
        To be used by op_handler to apply a stabilizer / error instruction directly
        to the TableauSimulator. Append to the new reference circuit if being constructed.
        """
        if isinstance(op, stim.Circuit):
            self._new_circuit += op
            if self._construct_reference_circuit:
                self._new_reference_circuit += op
            if self._running_tableau:
                self._tableau_simulator.do_circuit(op)
        else:
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
        if isinstance(op, stim.Circuit):
            self._new_circuit += op
            if self._running_tableau:
                self._tableau_simulator.do_circuit(op)
        else:
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

    def peek_bloch(self, target: int) -> stim.PauliString:
        self._run_tableau()
        return self._tableau_simulator.peek_bloch(target)

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
        if self._final_measurement_records is not None:
            return self._final_measurement_records
        if self._running_tableau and self.batch_size == 1:
            self._final_measurement_records = np.array([self.current_tableau_measurement_record()])
        else:
            self._final_measurement_records = self._new_circuit.compile_sampler(
                    seed=int(self._rng.integers(0, 2**31))
                ).sample(shots=self.batch_size, bit_packed=False)
            for meas_idx, p0, p1, is_inverted in self._asymmetric_readout_postproc:
                col = self._final_measurement_records[:, meas_idx]
                raw_bit = ~col if is_inverted else col
                prob_flip = np.where(raw_bit, 1.0 - p1, p0)
                flips = self.np_rng.random(self.batch_size) < prob_flip
                self._final_measurement_records[:, meas_idx] ^= flips
        return self._final_measurement_records
    
    def _convert_measurements_to_detector_flips(self) -> None:
        if not self._finished_running_circuit:
            raise RuntimeError("Have not finished running the circuit.")
        if self._final_measurement_records is None:
            if self.batch_size == 1 or self._asymmetric_readout_postproc:
                self.get_final_measurement_records()
            else:
                self._detector_flips, self._observable_flips = (
                    self._new_circuit.compile_detector_sampler(
                        seed=int(self._rng.integers(0, 2**31))
                    ).sample(
                       shots=self.batch_size, separate_observables=True, bit_packed=False
                    )
                )
                return
        self._detector_flips, self._observable_flips = self._compiled_m2d_converter.convert(
            measurements=self._final_measurement_records,
            separate_observables=True,
        )

    def get_detector_flips(self, append_observables=False) -> NDArray[np.bool_]:
        if self._detector_flips is None:
            self._convert_measurements_to_detector_flips()
        if append_observables and self._observable_flips is not None:
            return np.hstack([self._detector_flips, self._observable_flips])
        return self._detector_flips

    def get_observable_flips(self) -> NDArray[np.bool_]:
        if self._observable_flips is None:
            self._convert_measurements_to_detector_flips()
        return self._observable_flips