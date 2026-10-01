import itertools as it
from collections.abc import Iterable
from typing import Literal

import numpy as np
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.util.known_states import (
    _unroll_circuit,
    compute_known_states_uint8,
    convert_paulis_to_arrays,
)
from stimside.util.numpy_types import Bool1DArray, Bool2DArray
from stimside.util.unleaked_to_leaked import UnleakedToLeakedRecordsMixin


def _bit_transpose_swar(df_pack: np.ndarray, batch_size: int) -> np.ndarray:
    """Fast 64-bit SWAR 8x8 bit-transpose from (num_items, ceil(batch_size/8))
    to (batch_size, ceil(num_items/8)) little-endian bit-packed uint8.
    """
    num_items, b_bytes = df_pack.shape
    d_bytes = (num_items + 7) // 8
    if num_items == 0 or batch_size == 0:
        return np.zeros((batch_size, d_bytes), dtype=np.uint8)
    if num_items % 8 != 0:
        padded = np.zeros((d_bytes * 8, b_bytes), dtype=np.uint8)
        padded[:num_items, :] = df_pack
    else:
        padded = df_pack
    blocks = np.ascontiguousarray(
        padded.reshape(d_bytes, 8, b_bytes).transpose(2, 0, 1)
    )
    x = blocks.view(np.uint64)
    t = (x ^ (x >> np.uint64(7))) & np.uint64(0x00AA00AA00AA00AA)
    x = x ^ t ^ (t << np.uint64(7))
    t = (x ^ (x >> np.uint64(14))) & np.uint64(0x0000CCCC0000CCCC)
    x = x ^ t ^ (t << np.uint64(14))
    t = (x ^ (x >> np.uint64(28))) & np.uint64(0x00000000F0F0F0F0)
    x = x ^ t ^ (t << np.uint64(28))
    out = (
        x.view(np.uint8)
        .reshape(b_bytes, d_bytes, 8)
        .transpose(0, 2, 1)
        .reshape(b_bytes * 8, d_bytes)
    )
    return out[:batch_size, :]


class FlipsideSimulator(UnleakedToLeakedRecordsMixin):
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
        record_unleaked_to_leaked: bool = False,
    ) -> None:
        """Initialize FlipsideSimulator.

        Args:
            circuit: The Stim circuit to simulate.
            compiled_op_handler: Compiled handler for custom/noisy operations.
            batch_size: Number of shots to simulate in parallel.
            compute_known_states: Whether to precompute noiseless known Pauli states.
            disable_stabilizer_randomization: Disable random stabilizer initialization in FlipSimulator.
            seed: Optional RNG seed.
            record_unleaked_to_leaked: If True, record only transitions from an unleaked
                state (state < 2, i.e., U, 0, 1) to a leaked state (state >= 2) for each shot.
                Each transition is labeled by its 0-based unrolled circuit instruction index
                (`op_idx` into `_unroll_circuit(circuit)`, with REPEAT blocks unrolled).
        """

        self.circuit = circuit
        self._unrolled_ops: list[stim.CircuitInstruction] = _unroll_circuit(circuit)
        self.num_qubits = circuit.num_qubits

        self.seed = seed
        self.record_unleaked_to_leaked = bool(record_unleaked_to_leaked)
        self._unleaked_to_leaked_events: list[list[int]] = (
            [[] for _ in range(batch_size)]
            if self.record_unleaked_to_leaked
            else []
        )
        self._batches_completed = 0

        self._flip_simulator = stim.FlipSimulator(
            batch_size=batch_size,
            disable_stabilizer_randomization=disable_stabilizer_randomization,
            num_qubits=self.num_qubits,
            seed=self.seed,
        )

        self.np_rng = np.random.default_rng(seed=seed)
        self._reference_sample: np.ndarray | None = None

        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self.compiled_op_handler = compiled_op_handler

        if compute_known_states:
            self._known_states_uint8: np.ndarray | None = compute_known_states_uint8(
                self.circuit, unrolled_ops=self._unrolled_ops
            )
        else:
            self._known_states_uint8 = None

        self._circuit_time = 0
        self._used_interactively = False

    def _precompute_sync_reference(self) -> None:
        if self._reference_sample is not None:
            return
        ref_ts = stim.TableauSimulator(seed=0)
        ref_ts.set_num_qubits(self.num_qubits)
        ref_chunks: list[np.ndarray] = []
        for op in self._unrolled_ops:
            name = op.name
            if name == "MPAD":
                crs = np.array(
                    [
                        bool(gt.value) ^ bool(gt.is_inverted_result_target)
                        for gt in op.targets_copy()
                    ],
                    dtype=np.bool_,
                )
                ref_chunks.append(crs)
            elif name in ("HERALDED_ERASE", "HERALDED_PAULI_CHANNEL_1"):
                crs = np.zeros(len(op.targets_copy()), dtype=np.bool_)
                ref_chunks.append(crs)
            elif name in (
                "M",
                "MZ",
                "MR",
                "MRZ",
                "MX",
                "MRX",
                "MY",
                "MRY",
                "R",
                "RZ",
                "RX",
                "RY",
            ):
                is_x = name in ("MX", "MRX", "RX")
                is_y = name in ("MY", "MRY", "RY")
                is_r = name in (
                    "MR",
                    "MRZ",
                    "MRX",
                    "MRY",
                    "R",
                    "RZ",
                    "RX",
                    "RY",
                )
                produces_m = name in (
                    "M",
                    "MZ",
                    "MR",
                    "MRZ",
                    "MX",
                    "MRX",
                    "MY",
                    "MRY",
                )
                crs_list: list[bool] = []
                for gt in op.targets_copy():
                    q = gt.qubit_value if gt.qubit_value is not None else gt.value
                    if is_x:
                        ref_ts.h(q)
                    elif is_y:
                        ref_ts.h_yz(q)
                    m_val = ref_ts.measure(q)
                    if is_r:
                        ref_ts.reset_z(q)
                    if is_x:
                        ref_ts.h(q)
                    elif is_y:
                        ref_ts.h_yz(q)
                    crs_list.append(
                        bool(m_val) ^ bool(gt.is_inverted_result_target)
                    )
                if produces_m:
                    ref_chunks.append(np.array(crs_list, dtype=np.bool_))
            elif name in ("MXX", "MYY", "MZZ", "MPP"):
                raw_targets = op.targets_copy()
                terms: list[list[stim.GateTarget]] = []
                if name == "MPP":
                    t_i = 0
                    while t_i < len(raw_targets):
                        term = [raw_targets[t_i]]
                        t_i += 1
                        while (
                            t_i < len(raw_targets)
                            and raw_targets[t_i].is_combiner
                        ):
                            term.append(raw_targets[t_i + 1])
                            t_i += 2
                        terms.append(term)
                else:
                    for t_i in range(0, len(raw_targets), 2):
                        terms.append([raw_targets[t_i], raw_targets[t_i + 1]])

                crs_list = []
                for term in terms:
                    inv_res = False
                    pre_ops: list[stim.CircuitInstruction] = []
                    post_ops: list[stim.CircuitInstruction] = []
                    term_paulis: list[tuple[int, str]] = []
                    if name == "MPP":
                        for gt in term:
                            q_r = gt.qubit_value
                            if gt.is_inverted_result_target:
                                inv_res = not inv_res
                            if gt.is_x_target:
                                p_str = "X"
                                h_op = stim.CircuitInstruction("H", [q_r])
                                pre_ops.append(h_op)
                                post_ops.insert(0, h_op)
                            elif gt.is_y_target:
                                p_str = "Y"
                                hyz_op = stim.CircuitInstruction("H_YZ", [q_r])
                                pre_ops.append(hyz_op)
                                post_ops.insert(0, hyz_op)
                            else:
                                p_str = "Z"
                            term_paulis.append((q_r, p_str))
                    else:
                        p_str = name[-1]
                        q0, q1 = term[0].qubit_value, term[1].qubit_value
                        inv_res = bool(
                            term[0].is_inverted_result_target
                            ^ term[1].is_inverted_result_target
                        )
                        term_paulis = [(q0, p_str), (q1, p_str)]
                        if p_str == "X":
                            h_op = stim.CircuitInstruction("H", [q0, q1])
                            pre_ops.append(h_op)
                            post_ops.append(h_op)
                        elif p_str == "Y":
                            hyz_op = stim.CircuitInstruction("H_YZ", [q0, q1])
                            pre_ops.append(hyz_op)
                            post_ops.append(hyz_op)

                    q0 = term_paulis[0][0]
                    cx_ops: list[stim.CircuitInstruction] = []
                    for q_r, _ in term_paulis[1:]:
                        cx_ops.append(stim.CircuitInstruction("CX", [q_r, q0]))
                    pre_ops.extend(cx_ops)
                    post_ops = list(reversed(cx_ops)) + post_ops

                    for p_op in pre_ops:
                        ref_ts.do(p_op)
                    m_val = ref_ts.measure(q0)
                    for p_op in post_ops:
                        ref_ts.do(p_op)
                    crs_list.append(bool(m_val) ^ inv_res)

                ref_chunks.append(np.array(crs_list, dtype=np.bool_))
            else:
                gd = stim.gate_data(name)
                if gd.is_unitary or gd.is_reset:
                    if name not in ("I", "II", "I_ERROR", "II_ERROR"):
                        ref_ts.do(op)
        if ref_chunks:
            self._reference_sample = np.concatenate(ref_chunks)
        else:
            self._reference_sample = np.empty(0, dtype=np.bool_)

    @property
    def ref_measurements(self) -> np.ndarray:
        self._precompute_sync_reference()
        assert self._reference_sample is not None
        return self._reference_sample

    ############################################################################
    # Public methods without running interactively
    ############################################################################

    def run(self) -> list[np.ndarray] | None:
        """Run the simulator and populate with data.

        When `record_unleaked_to_leaked=True`, returns a list of length `batch_size`
        where element `b` is a 1D `np.ndarray` (`dtype=np.int64`) of the unrolled
        circuit operation indices (`op_idx` into `_unroll_circuit(circuit)`) where an
        unleaked-to-leaked transition occurred in shot `b`.
        """
        if self._used_interactively or self._circuit_time != 0:
            raise ValueError(
                "FlipsideSimulator is not in a clean state as it has been used interactively."
                "Use .clear() to prepare the simulator for a new run. "
            )
        for op in self._unrolled_ops:
            self._do_instruction(op)
        self._batches_completed += 1
        if self.record_unleaked_to_leaked:
            return self.get_unleaked_to_leaked_records()
        return None

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

        if self.record_unleaked_to_leaked:
            self._unleaked_to_leaked_events = [
                [] for _ in range(self.batch_size)
            ]

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

    def get_detector_flips(self, *args, bit_packed: bool = False, **kwargs):
        if bit_packed:
            raw_packed = self._flip_simulator.get_detector_flips(
                *args, bit_packed=True, **kwargs
            )
            return _bit_transpose_swar(raw_packed, self.batch_size)
        return self._flip_simulator.get_detector_flips(
            *args, bit_packed=False, **kwargs
        )

    def get_measurement_flips(self, *args, **kwargs):
        return self._flip_simulator.get_measurement_flips(*args, **kwargs)

    def get_final_measurement_records(self) -> np.ndarray:
        """Return measurement records of shape (batch_size, num_measurements)."""
        self._precompute_sync_reference()
        assert self._reference_sample is not None
        flips = self.get_measurement_flips()
        return np.logical_xor(self._reference_sample[:, None], flips).T

    def get_observable_flips(self, *args, bit_packed: bool = False, **kwargs):
        if bit_packed:
            raw_packed = self._flip_simulator.get_observable_flips(
                *args, bit_packed=True, **kwargs
            )
            return _bit_transpose_swar(raw_packed, self.batch_size)
        return self._flip_simulator.get_observable_flips(
            *args, bit_packed=False, **kwargs
        )

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
        """
        if p <= 0.0 or not np.any(error_mask):
            return error_mask
        self._flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=error_mask, p=p)
        return error_mask

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
        view = array.reshape(-1)
        if view.base is not array:
            raise ValueError(
                "received an array that numpy cannot reshape without making a copy. "
            )
        self._flip_simulator.generate_bernoulli_samples(
            p=p, num_samples=np.prod(view.shape), bit_packed=False, out=view
        )

    ############################################################################
    # known states methods:
    ############################################################################

    def _get_all_clean_known_state(
        self, circuit_time: int | None = None
    ) -> tuple[
        Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
    ]:
        """given circuit locations, return the state of the target under noiseless execution."""
        if self._known_states_uint8 is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        return convert_paulis_to_arrays(self._known_states_uint8[circuit_time])

    _PAULI_TO_UINT8_CODE = {"X": 1, "Y": 3, "Z": 5}

    def _get_clean_known_states(
        self, pauli: Literal["X", "Y", "Z"], *, circuit_time: int | None = None
    ) -> tuple[Bool1DArray, Bool1DArray]:
        """given circuit locations, return the state of the target under noiseless execution."""
        if self._known_states_uint8 is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        row = self._known_states_uint8[circuit_time]
        code = self._PAULI_TO_UINT8_CODE[pauli]
        return (row == code, row == (code + 1))

    def _all_targets_in_known_state(
        self,
        targets: Iterable[int],
        pauli: Literal["X", "Y", "Z"],
        circuit_time: int | None = None,
    ) -> bool:
        """returns true if all targets are in a known state of the given pauli."""
        if self._known_states_uint8 is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        row = self._known_states_uint8[circuit_time]
        code = self._PAULI_TO_UINT8_CODE[pauli]
        return all(row[int(t)] == code or row[int(t)] == (code + 1) for t in targets)

    def _get_current_known_state_masks(
        self, pauli: Literal["X", "Y", "Z"], *, circuit_time: int | None = None
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
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        clean_plus, clean_minus = self._get_clean_known_states(
            pauli=pauli, circuit_time=circuit_time
        )
        clean_plus = clean_plus.reshape(-1, 1)
        clean_minus = clean_minus.reshape(-1, 1)

        xs, zs, _, _, _ = self._flip_simulator.to_numpy(
            output_xs=(pauli in ("Y", "Z")),
            output_zs=(pauli in ("X", "Y")),
        )
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
            for op in this:
                self._do(op)
        elif isinstance(this, stim.CircuitRepeatBlock):
            loop_body = this.body_copy()
            for _ in range(this.repeat_count):
                self._do(loop_body)
        elif isinstance(this, stim.CircuitInstruction):
            self._do_instruction(this)
        else:
            raise NotImplementedError

    def _do_instruction(self, op: stim.CircuitInstruction):
        """for each instruction, update the state of the simulator."""
        if op.name == "QUBIT_COORDS":
            [gt] = op.targets_copy()
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

