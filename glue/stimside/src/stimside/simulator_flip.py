import itertools as it
from collections.abc import Iterable
from typing import Literal

import numpy as np
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.util.known_states import compute_known_states, convert_paulis_to_arrays
from stimside.util.numpy_types import Bool1DArray, Bool2DArray


class FlipsideSimulator:
    """A fast and convenient correlated error simulator based on stim.FlipSimulator.

    Inherits the advantages and disadvantages of a flip simulator:
     - much faster than a tableau simulator (O(1) cost for gates, rather than O(?))
     - restricted to handling Pauli errors (No clifford errors, no 'CZ gate doesn't occur')

    The FlipsideSimulator has three major additions over the stim.FlipSimulator:
    1. Extra methods and state to help use a FlipSimulator:
        Methods include those for making and using random bitmasks, broadcasting Pauli errors, etc.
            (These methods are typically candidates for eventually being put into the FlipSimulator)
        State includes tracking the qubit coordinate for use in implementing errors

    2. Additional information regarding the circuit being simulated, particularly:
        - The qubit coordinates (including the effects of SHIFT_COORDS)
            This lets the simulator infer where qubits are, allowing behaviour (like errors)
            to depend on location. (See CompiledOpHandler for a way of adding such behaviour)
        - The 'known states' of the qubits in the circuit:
            This is a record of all locations in the circuit where we can infer the single qubit state,
            which is not usually possible for a flip simulator. Basically, this amounts to all places
            where qubits are known to be unentangled, such as immediately next to M and R operations.
            We compute that actual known state using the reference sample for the circuit.

    3. CompiledOpHandler, which let the simulator implement more complex behaviours:
        Lets the simulator hold configurable state, and provides an easy way of registering
        repeatable behaviours that depend on the capabilities provides above.

        We take a CompiledOpHandler rather than a OpHandler that we then compile to try
        to simplify the class and to encourage users not to do any slow work inside the simulator.
        You are of course free to implement a CompiledOpHandler by hand rather than by making a
        OpHandler, especially when you're using the simulator outside `sinter.collect`

        We take a CompiledOpHandler at all in order to help guide the dividing line between the
        simulator itself and the custom behaviour the user wants. Two rules of thumb are:
         - everything that could one day be included in the stim.FlipSimulator is in the FlipsideSimulator,
         - everything that is hardware or noise model dependent is in the CompiledOpHandler
    """

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        compiled_op_handler: CompiledOpHandler["FlipsideSimulator"],
        batch_size: int,
        compute_known_states: bool = True,
        disable_stabilizer_randomization: bool = False,
        seed: int | None = None,
    ) -> None:

        self.circuit = circuit
        self.num_qubits = circuit.num_qubits

        self.seed = seed

        self._flip_simulator = stim.FlipSimulator(
            batch_size=batch_size,
            disable_stabilizer_randomization=disable_stabilizer_randomization,
            num_qubits=self.num_qubits,
            seed=self.seed,
        )

        self.np_rng = np.random.default_rng(seed=seed)
        # this exists so that op_handlers can access random numbers other than
        # via the Bernoulli functions that stim provides
        # there's a decent argument that they should make the rng themselves if they want one
        # TODO: reflect upon this

        self.qubit_coords: dict[int, list[float]] = {}
        # qubit_coords is a mapping of stim index --> stim args to the QUBIT_COORD instruction

        self.qubit_tags: dict[int, str] = {}
        # qubit_tags is a mapping of  stim index --> tag string for the QUBIT_COORD instruction

        self.coords_shifts: list[float] = []
        # list of ints to track total shifts from SHIFT_COORD instructions

        self.compiled_op_handler = compiled_op_handler

        self._known_states: list | None = (
            self._compute_known_states() if compute_known_states else None
        )

        self._circuit_time = 0
        self._used_interactively = False

    ############################################################################
    # Public methods without running interactively
    ############################################################################

    def run(self):
        """run the simulator and populate with data."""
        if self._used_interactively or self._circuit_time != 0:
            raise ValueError(
                "FlipsideSimulator is not in a clean state as it has been used interactively."
                "Use .clear() to prepare the simulator for a new run. "
            )
        self._do(self.circuit)

    def clear(self):
        """clear the simulator so it can be reused.

        This is performance critical, as it's going to be used by the sampler before every batch.
        Avoid just recreating the simulator, we shouldn't have to redo any configuration steps here
        """
        self._flip_simulator.clear()

        # clear qubit coords, as these can change during the circuit
        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self.compiled_op_handler.clear()

        self._circuit_time = 0
        self._used_interactively = False

    def get_current_circuit_time(self) -> int:
        return self._circuit_time

    ############################################################################
    # flip simulator lookthrough methods
    ############################################################################
    @property
    def batch_size(self):
        return self._flip_simulator.batch_size

    def get_detector_flips(self, *args, **kwargs):
        return self._flip_simulator.get_detector_flips(*args, **kwargs)

    def get_measurement_flips(self, *args, **kwargs):
        return self._flip_simulator.get_measurement_flips(*args, **kwargs)

    def get_observable_flips(self, *args, **kwargs):
        return self._flip_simulator.get_observable_flips(*args, **kwargs)

    def broadcast_pauli_errors(
        self, error_mask: Bool2DArray, p: float, pauli: Literal["X", "Y", "Z"] = "X"
    ):
        """Apply errors in parallel over qubits and simulation instances.

        Args:
            error_mask: a 2D bool array with the same shape as the simulator flip states
                i.e. (self.max_qubit_idx, self.old_leakage_flip_simulator.batch_size)
                An error will be applied to qubit q in instance k if
                    error_mask[q, k] == True
            p: An optional probability to keep each True value in error_mask
                i.e. each bool in error mask is AND'd with a bool that is True with probability p
            pauli: which Pauli error to broadcast over the flip states
                'Z' applied Pauli Z flips, 'X' applies Pauli X flips
                'Y' applies both Pauli Z and X flips

        returns a mask indicating which errors were applied
            if p=1, the input error_mask array is returned
            else, a new array is returned with the same shape and dtype as error_mask
        """
        self._flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=error_mask, p=p)

    def do_on_flip_simulator(
        self, obj: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock
    ):
        self._flip_simulator.do(obj=obj)

    def append_measurement_flips(self, measurement_flip_data: np.ndarray):
        self._flip_simulator.append_measurement_flips(
            measurement_flip_data=measurement_flip_data
        )

    ############################################################################
    # mask methods: make and manipulate random masks (2D bool arrays)
    ############################################################################

    def _make_random_mask(self, p: float, shape: tuple[int, int]) -> Bool2DArray:
        """returns a new mask (a 2D bool array of the given shape)

        Each element is true with probability p
        """
        out = self._flip_simulator.generate_bernoulli_samples(
            p=p, num_samples=np.prod(shape), bit_packed=False, out=None
        )
        return out.reshape(shape)

    def _fill_random_mask(self, p: float, array: Bool2DArray) -> None:
        """given an existing mask, fill it with new random samples.

        because generate_bernoulli_samples demands a 1D bool array, this is only
        allowed if the array you're handing in is contiguous and can be reshaped to 1D
        without making a copy.
        """
        view = array.reshape(-1)  # this could make a copy, in which case we're boned
        if view.base is not array:
            raise ValueError(
                f"received an array that numpy cannot reshape without making a copy. "
            )
        self._flip_simulator.generate_bernoulli_samples(
            p=p, num_samples=np.prod(view.shape), bit_packed=False, out=view
        )

    ############################################################################
    # known states methods:
    ############################################################################

    def _compute_known_states(
        self,
    ) -> list[
        tuple[
            Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
        ]
    ]:
        """init method for actually building the known states structure,

        known_states is a record of every circuit location with a known single qubit state.
        """
        known_pauli_strings = compute_known_states(self.circuit)
        return [convert_paulis_to_arrays(ps) for ps in known_pauli_strings]

    def _get_all_clean_known_state(
        self, circuit_time: int | None = None
    ) -> tuple[
        Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
    ]:
        """given circuit locations, return the state of the target under noiseless execution.

        Args:
            circuit_time: a unique gap between instructions as the circuit
            is executed. Basically, the moment in time right before the instruction
            circuit.flattened()[circuit_time]

        Returns:
            Returns a set of bool arrays of shape (num_qubits, )
            In order these are:
                in_pX: True if qubit is in +X
                in_mX: True if qubit is in -X
                in_pY: True if qubit is in +Y
                in_mY: True if qubit is in -Y
                in_pZ: True if qubit is in +Z
                in_mZ: True if qubit is in -Z
        """
        if self._known_states is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        circuit_time = circuit_time or self.get_current_circuit_time()
        return self._known_states[circuit_time]

    _PAULI_TO_KNOWN_STATES_INDEX = {"X": 0, "Y": 2, "Z": 4}

    def _get_clean_known_states(
        self, pauli: Literal["X", "Y", "Z"], *, circuit_time: int | None = None
    ) -> tuple[Bool1DArray, Bool1DArray]:
        """given circuit locations, return the state of the target under noiseless execution.

        Args:
            circuit_time: a unique gap between instructions as the circuit
            is executed. Basically, the moment in time right before the instruction
            circuit.flattened()[circuit_time]
            pauli: decides which pauli P out of X, Y, Z we return the arrays for

        Returns:
            Returns a set of bool arrays of shape (num_qubits, )
            In order these are:
                in_pP: True if qubit is in +P
                in_mP: True if qubit is in -P
        """
        if self._known_states is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        circuit_time = circuit_time or self.get_current_circuit_time()
        idx = self._PAULI_TO_KNOWN_STATES_INDEX[pauli]
        return (
            self._known_states[circuit_time][idx],
            self._known_states[circuit_time][idx + 1],
        )

    def _all_targets_in_known_state(
        self,
        targets: Iterable[int],
        pauli: Literal["X", "Y", "Z"],
        circuit_time: int | None = None,
    ) -> bool:
        """returns true of all targets are in a known state of the given pauli."""
        in_plus, in_minus = self._get_clean_known_states(
            pauli=pauli, circuit_time=circuit_time
        )
        in_known = np.logical_or(in_plus, in_minus)
        return all(in_known[t] for t in targets)

    def _get_current_known_state_masks(
        self, pauli: Literal["X", "Y", "Z"]
    ) -> tuple[Bool2DArray, Bool2DArray]:
        """given circuit locations, return masks indicating the qubits known states.

        Returns two masks, for qubits found to be in the +1 and -1 eigenstate respectively

        Args:
            pauli: which pauli to filter the known state for

        Returns:
            Two bool fields of shape (num_qubits, batch_size):
                in_pP: in_pP[q, k] is True if the qubit q in instance k is in +P
                in_mP: in_pP[q, k] is True if the qubit q in instance k is in -P
        """

        clean_plus, clean_minus = self._get_clean_known_states(
            pauli=pauli, circuit_time=self.get_current_circuit_time()
        )
        # reshape from (num_qubits,) to (num_qubits,1) to make the broadcasting easier
        clean_plus = clean_plus.reshape(-1, 1)
        clean_minus = clean_minus.reshape(-1, 1)

        xs, zs, _, _, _ = self._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        match pauli:
            case "X":
                is_flipped = zs
            case "Y":
                is_flipped = np.logical_xor(xs, zs)
            case "Z":
                is_flipped = xs
            case _:
                raise ValueError("Unrecognised Pauli")

        is_not_flipped = np.logical_not(is_flipped)

        in_plus_clean = np.logical_and(clean_plus, is_not_flipped)
        in_plus_flipped = np.logical_and(clean_minus, is_flipped)
        in_minus_clean = np.logical_and(clean_minus, is_not_flipped)
        in_minus_flipped = np.logical_and(clean_plus, is_flipped)

        is_plus = np.logical_or(in_plus_clean, in_plus_flipped)
        is_minus = np.logical_or(in_minus_clean, in_minus_flipped)

        return is_plus, is_minus

    ############################################################################
    # do methods: run the simulation
    ############################################################################
    def interactive_do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock, /
    ):
        self._used_interactively = True
        self._do(this)

    def _do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock, /
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
        else:
            raise NotImplementedError

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

        elif op.name == "SHIFT_COORDS":
            shifts = op.gate_args_copy()
            for i, s in enumerate(shifts):
                if i >= len(self.coords_shifts):
                    self.coords_shifts.append(s)
                else:
                    self.coords_shifts[i] += s

        self.compiled_op_handler.handle_op(op=op, sss=self)
        self._circuit_time += 1
