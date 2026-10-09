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


class _KnownStatesView:
    """Lightweight view over a (num_ops + 1, num_qubits) uint8 table that
    returns the 6-tuple of boolean arrays on demand when indexed.
    """

    def __init__(self, table_uint8: np.ndarray) -> None:
        self._table = table_uint8

    def __len__(self) -> int:
        return int(self._table.shape[0])

    def __getitem__(self, idx: int):
        return convert_paulis_to_arrays(self._table[idx])


class FlipsideSimulator:
    """A fast and convenient correlated error simulator based on stim.FlipSimulator."""

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        compiled_op_handler: CompiledOpHandler["FlipsideSimulator"],
        batch_size: int,
        compute_known_states: bool = True,
        disable_stabilizer_randomization: bool = False,
        seed: int | None = None,
        sync_tableside_rng: bool = False,
    ) -> None:

        self.circuit = circuit
        self._unrolled_ops: list[stim.CircuitInstruction] = _unroll_circuit(circuit)
        self.num_qubits = circuit.num_qubits

        self.seed = seed
        self.sync_tableside_rng = bool(sync_tableside_rng)
        self._batches_completed = 0
        self._tab_scratch_q = self.num_qubits
        self._shot_np_rngs: list[np.random.Generator] | None = None
        self._tab_rngs: list[stim.TableauSimulator] | None = None
        self._tab_rngs_x: list[stim.TableauSimulator] | None = None
        self._sync_paired_controlled_error: bool = True

        eff_disable_stab_rand = (
            True if self.sync_tableside_rng else disable_stabilizer_randomization
        )

        self._flip_simulator = stim.FlipSimulator(
            batch_size=batch_size,
            disable_stabilizer_randomization=eff_disable_stab_rand,
            num_qubits=self.num_qubits,
            seed=self.seed,
        )

        self.np_rng = np.random.default_rng(seed=seed)
        self._scratch_x_mask = np.zeros(
            (self.num_qubits, batch_size), dtype=np.bool_
        )
        self._scratch_z_mask = np.zeros(
            (self.num_qubits, batch_size), dtype=np.bool_
        )
        self._step_kickbacks: dict[
            int, list[tuple[np.ndarray, np.ndarray] | None]
        ] = {}
        self._step_clean_refs: dict[int, np.ndarray] = {}
        self._reference_sample: np.ndarray | None = None

        if self.sync_tableside_rng:
            self._init_sync_rngs()
            self._precompute_sync_reference()

        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self.compiled_op_handler = compiled_op_handler

        if compute_known_states:
            self._known_states_uint8: np.ndarray | None = compute_known_states_uint8(
                self.circuit, unrolled_ops=self._unrolled_ops
            )
            self._known_states: _KnownStatesView | list | None = _KnownStatesView(
                self._known_states_uint8
            )
        else:
            self._known_states_uint8 = None
            self._known_states = None

        self._circuit_time = 0
        self._used_interactively = False

    def _init_sync_rngs(self) -> None:
        if not self.sync_tableside_rng:
            return
        base_seed = (
            0
            if self.seed is None
            else int(self.seed) + self._batches_completed * self.batch_size
        )
        self._shot_np_rngs = [
            np.random.default_rng(seed=base_seed + b)
            for b in range(self.batch_size)
        ]
        self.np_rng = self._shot_np_rngs[0]
        self._tab_rngs = []
        self._tab_rngs_x = []
        all_data_q = list(range(self.num_qubits))
        for b in range(self.batch_size):
            ts_z = stim.TableauSimulator(seed=base_seed + b)
            ts_z.set_num_qubits(self.num_qubits + 1)
            self._tab_rngs.append(ts_z)

            ts_x = stim.TableauSimulator(seed=base_seed + b)
            ts_x.set_num_qubits(self.num_qubits + 1)
            if all_data_q:
                ts_x.h(*all_data_q)
            self._tab_rngs_x.append(ts_x)

    def _precompute_sync_reference(self) -> None:
        if self._reference_sample is not None:
            return
        ref_ts = stim.TableauSimulator(seed=0)
        ref_ts.set_num_qubits(self.num_qubits)
        ref_chunks: list[np.ndarray] = []
        for idx, op in enumerate(self._unrolled_ops):
            name = op.name
            if name == "MPAD":
                crs = np.array(
                    [bool(gt.value) for gt in op.targets_copy()], dtype=np.bool_
                )
                self._step_kickbacks[idx] = [None] * len(crs)
                self._step_clean_refs[idx] = crs
                ref_chunks.append(crs)
            elif name in ("M", "MZ", "MR", "MRZ", "MX", "MRX", "MY", "MRY"):
                is_x = name in ("MX", "MRX")
                is_y = name in ("MY", "MRY")
                is_r = name in ("MR", "MRZ", "MRX", "MRY")
                ks: list[tuple[np.ndarray, np.ndarray] | None] = []
                crs_list: list[bool] = []
                for gt in op.targets_copy():
                    q = gt.qubit_value if gt.qubit_value is not None else gt.value
                    if is_x:
                        ref_ts.h(q)
                    elif is_y:
                        ref_ts.h_yz(q)
                    m_val, kb = ref_ts.measure_kickback(q)
                    if kb is None:
                        ks.append(None)
                    else:
                        kb_x, kb_z = kb.to_numpy()
                        if is_x and q < len(kb_x):
                            kb_x[q], kb_z[q] = kb_z[q], kb_x[q]
                        elif is_y and q < len(kb_x):
                            kb_y_q = kb_z[q]
                            kb_z[q] = kb_x[q]
                            kb_x[q] = kb_y_q
                        ks.append(
                            (
                                np.where(kb_x)[0].astype(np.intp),
                                np.where(kb_z)[0].astype(np.intp),
                            )
                        )
                    if is_r:
                        ref_ts.reset_z(q)
                    if is_x:
                        ref_ts.h(q)
                    elif is_y:
                        ref_ts.h_yz(q)
                    crs_list.append(
                        bool(m_val) ^ bool(gt.is_inverted_result_target)
                    )
                crs_arr = np.array(crs_list, dtype=np.bool_)
                self._step_kickbacks[idx] = ks
                self._step_clean_refs[idx] = crs_arr
                ref_chunks.append(crs_arr)
            else:
                gd = stim.gate_data(name)
                if gd.is_unitary or gd.is_reset:
                    if name not in ("I", "II", "I_ERROR", "II_ERROR"):
                        ref_ts.do(op)
        if ref_chunks:
            self._reference_sample = np.concatenate(ref_chunks)
        else:
            self._reference_sample = np.empty(0, dtype=np.bool_)

    def _draw_tab_rng_bit(self, b_idx: int) -> bool:
        assert self._tab_rngs is not None and self._tab_rngs_x is not None
        tr_z = self._tab_rngs[b_idx]
        tr_x = self._tab_rngs_x[b_idx]
        sq = self._tab_scratch_q
        tr_z.h(sq)
        bit = bool(tr_z.measure(sq))
        if bit:
            tr_z.x(sq)
        tr_x.h(sq)
        bit_x = bool(tr_x.measure(sq))
        if bit_x:
            tr_x.x(sq)
        return bit

    @property
    def _sync_np_rngs(self) -> list[np.random.Generator]:
        assert self._shot_np_rngs is not None
        return self._shot_np_rngs

    @property
    def ref_measurements(self) -> np.ndarray:
        self._precompute_sync_reference()
        assert self._reference_sample is not None
        return self._reference_sample

    def _sample_pauli_noise_via_tab_rng(
        self,
        op_name_or_b: str | int,
        targets_per_shot_or_op: list[list[int]] | list[np.ndarray] | str,
        gate_args: list[float],
        single_shot_targets: list[int] | None = None,
    ) -> None:
        assert self._tab_rngs is not None and self._tab_rngs_x is not None
        if isinstance(op_name_or_b, int):
            b_idx = int(op_name_or_b)
            op_name = str(targets_per_shot_or_op)
            assert single_shot_targets is not None
            if len(single_shot_targets) == 0:
                return
            inst_b = stim.CircuitInstruction(
                op_name, single_shot_targets, gate_args
            )
            tr_z = self._tab_rngs[b_idx]
            tr_x = self._tab_rngs_x[b_idx]
            tr_z.do(inst_b)
            tr_x.do(inst_b)
            self._scratch_x_mask.fill(False)
            self._scratch_z_mask.fill(False)
            any_x = False
            any_z = False
            for q in set(int(x) for x in single_shot_targets):
                if tr_z.peek_z(q) == -1:
                    self._scratch_x_mask[q, b_idx] ^= True
                    tr_z.x(q)
                    any_x = True
                if tr_x.peek_x(q) == -1:
                    self._scratch_z_mask[q, b_idx] ^= True
                    tr_x.z(q)
                    any_z = True
            if any_x:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_x_mask, p=1.0
                )
            if any_z:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_z_mask, p=1.0
                )
            return

        op_name = str(op_name_or_b)
        targets_per_shot = targets_per_shot_or_op  # type: ignore[assignment]
        self._scratch_x_mask.fill(False)
        self._scratch_z_mask.fill(False)
        any_x = False
        any_z = False
        for b_idx in range(self.batch_size):
            t_b = targets_per_shot[b_idx]
            if len(t_b) == 0:
                continue
            inst_b = stim.CircuitInstruction(op_name, t_b, gate_args)
            tr_z = self._tab_rngs[b_idx]
            tr_x = self._tab_rngs_x[b_idx]
            tr_z.do(inst_b)
            tr_x.do(inst_b)
            for q in set(int(x) for x in t_b):
                if tr_z.peek_z(q) == -1:
                    self._scratch_x_mask[q, b_idx] ^= True
                    tr_z.x(q)
                    any_x = True
                if tr_x.peek_x(q) == -1:
                    self._scratch_z_mask[q, b_idx] ^= True
                    tr_x.z(q)
                    any_z = True
        if any_x:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=self._scratch_x_mask, p=1.0
            )
        if any_z:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=self._scratch_z_mask, p=1.0
            )

    def _apply_sync_measurement_kickbacks(
        self,
        op: stim.CircuitInstruction,
        circuit_time: int | None = None,
    ) -> np.ndarray:
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        self._precompute_sync_reference()
        ks = self._step_kickbacks[circuit_time]
        crs = self._step_clean_refs[circuit_time]
        if not any(kb is not None for kb in ks):
            return crs

        raw_targets = op.targets_copy()
        ts_targets = [
            gt.qubit_value if gt.qubit_value is not None else gt.value
            for gt in raw_targets
        ]
        inv_flags = [bool(gt.is_inverted_result_target) for gt in raw_targets]
        is_x = op.name in ("MX", "MRX")
        is_y = op.name in ("MY", "MRY")

        xs, zs, _, _, _ = self._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        self._scratch_x_mask.fill(False)
        self._scratch_z_mask.fill(False)
        any_x = False
        any_z = False

        for b_idx in range(self.batch_size):
            for k_i, (q, kb, cr, inv) in enumerate(
                zip(ts_targets, ks, crs, inv_flags)
            ):
                if kb is None:
                    continue
                bit = self._draw_tab_rng_bit(b_idx) ^ inv
                if is_x:
                    flip_bit = bool(zs[q, b_idx])
                elif is_y:
                    flip_bit = bool(xs[q, b_idx] ^ zs[q, b_idx])
                else:
                    flip_bit = bool(xs[q, b_idx])
                cur_m = bool(cr) ^ flip_bit
                if cur_m != bit:
                    kb_x_idx, kb_z_idx = kb
                    if len(kb_x_idx) > 0:
                        xs[kb_x_idx, b_idx] ^= True
                        self._scratch_x_mask[kb_x_idx, b_idx] ^= True
                        any_x = True
                    if len(kb_z_idx) > 0:
                        zs[kb_z_idx, b_idx] ^= True
                        self._scratch_z_mask[kb_z_idx, b_idx] ^= True
                        any_z = True

        if any_x:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=self._scratch_x_mask, p=1.0
            )
        if any_z:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=self._scratch_z_mask, p=1.0
            )
        return crs

    def _clear_sync_basis_phase_flips(
        self, op_name: str, raw_targets: list[stim.GateTarget]
    ) -> None:
        ts_targets = [
            gt.qubit_value if gt.qubit_value is not None else gt.value
            for gt in raw_targets
        ]
        if not ts_targets:
            return
        if op_name in ("R", "RZ", "M", "MZ", "MR", "MRZ"):
            _, zs, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=False, output_zs=True
            )
            self._scratch_z_mask.fill(False)
            self._scratch_z_mask[ts_targets, :] = zs[ts_targets, :]
            if np.any(self._scratch_z_mask):
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_z_mask, p=1.0
                )
        elif op_name in ("RX", "MX", "MRX"):
            xs, _, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=True, output_zs=False
            )
            self._scratch_x_mask.fill(False)
            self._scratch_x_mask[ts_targets, :] = xs[ts_targets, :]
            if np.any(self._scratch_x_mask):
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_x_mask, p=1.0
                )

    def _do_sync_bare_measurement(self, op: stim.CircuitInstruction) -> None:
        assert self._tab_rngs is not None and self._tab_rngs_x is not None
        self._apply_sync_measurement_kickbacks(op)
        args = op.gate_args_copy()
        raw_targets = op.targets_copy()
        if args and args[0] > 0.0:
            p_meas = args[0]
            ts_targets = [
                gt.qubit_value if gt.qubit_value is not None else gt.value
                for gt in raw_targets
            ]
            rev_targets = ts_targets[::-1]
            m_flips = np.zeros(
                (len(ts_targets), self.batch_size), dtype=np.bool_
            )
            for b_idx in range(self.batch_size):
                inst_err = stim.CircuitInstruction(
                    "X_ERROR", rev_targets, [p_meas]
                )
                tr_z = self._tab_rngs[b_idx]
                tr_x = self._tab_rngs_x[b_idx]
                tr_z.do(inst_err)
                tr_x.do(inst_err)
                for k_rev, q in enumerate(rev_targets):
                    if tr_z.peek_z(q) == -1:
                        m_flips[len(ts_targets) - 1 - k_rev, b_idx] ^= True
                        tr_z.x(q)
                    if tr_x.peek_x(q) == -1:
                        tr_x.z(q)
            is_x = op.name in ("MX", "MRX")
            is_y = op.name in ("MY", "MRY")
            xs, zs, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=(not is_x), output_zs=(is_x or is_y)
            )
            if is_x:
                base_flips = zs[ts_targets, :]
            elif is_y:
                base_flips = xs[ts_targets, :] ^ zs[ts_targets, :]
            else:
                base_flips = xs[ts_targets, :]
            self._flip_simulator.append_measurement_flips(
                measurement_flip_data=(base_flips ^ m_flips)
            )
            if op.name in ("MR", "MRZ"):
                self._flip_simulator.do(
                    stim.CircuitInstruction("R", raw_targets, [])
                )
            elif op.name == "MRX":
                self._flip_simulator.do(
                    stim.CircuitInstruction("RX", raw_targets, [])
                )
            elif op.name == "MRY":
                self._flip_simulator.do(
                    stim.CircuitInstruction("RY", raw_targets, [])
                )
        else:
            self._flip_simulator.do(op)
        self._clear_sync_basis_phase_flips(op.name, raw_targets)

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
        for op in self._unrolled_ops:
            self._do_instruction(op)
        self._batches_completed += 1

    def clear(self):
        """clear the simulator so it can be reused."""
        self._flip_simulator.clear()

        # clear qubit coords, as these can change during the circuit
        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        if self.sync_tableside_rng:
            self._init_sync_rngs()

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
        """Apply errors in parallel over qubits and simulation instances."""
        if p <= 0.0 or not np.any(error_mask):
            return error_mask
        self._flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=error_mask, p=p)
        return error_mask

    def do_on_flip_simulator(
        self, obj: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock
    ):
        if self.sync_tableside_rng and isinstance(obj, stim.CircuitInstruction):
            name = obj.name
            if name in ("M", "MZ", "MR", "MRZ", "MX", "MRX", "MY", "MRY"):
                self._do_sync_bare_measurement(obj)
                return
            gd = stim.gate_data(name)
            if gd.is_noisy_gate:
                args = obj.gate_args_copy()
                if name not in ("I_ERROR", "II_ERROR") or (
                    args and args[0] > 0.0
                ):
                    target_indices = [
                        t.qubit_value if t.qubit_value is not None else t.value
                        for t in obj.targets_copy()
                    ]
                    self._sample_pauli_noise_via_tab_rng(
                        name, [target_indices] * self.batch_size, args
                    )
                return
            self._flip_simulator.do(obj=obj)
            if gd.is_reset:
                self._clear_sync_basis_phase_flips(name, obj.targets_copy())
            return
        self._flip_simulator.do(obj=obj)

    def append_measurement_flips(self, measurement_flip_data: np.ndarray):
        self._flip_simulator.append_measurement_flips(
            measurement_flip_data=measurement_flip_data
        )

    ############################################################################
    # mask methods: make and manipulate random masks (2D bool arrays)
    ############################################################################

    def _make_random_mask(self, p: float, shape: tuple[int, int]) -> Bool2DArray:
        """returns a new mask (a 2D bool array of the given shape)."""
        out = self._flip_simulator.generate_bernoulli_samples(
            p=p, num_samples=np.prod(shape), bit_packed=False, out=None
        )
        return out.reshape(shape)

    def _fill_random_mask(self, p: float, array: Bool2DArray) -> None:
        """given an existing mask, fill it with new random samples."""
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

    def _compute_known_states(
        self,
    ) -> list[
        tuple[
            Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
        ]
    ]:
        """init method for actually building the known states structure."""
        table = compute_known_states_uint8(
            self.circuit, unrolled_ops=self._unrolled_ops
        )
        return [convert_paulis_to_arrays(row) for row in table]

    def _get_all_clean_known_state(
        self, circuit_time: int | None = None
    ) -> tuple[
        Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
    ]:
        """given circuit locations, return the state of the target under noiseless execution."""
        if self._known_states is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        return self._known_states[circuit_time]

    _PAULI_TO_KNOWN_STATES_INDEX = {"X": 0, "Y": 2, "Z": 4}
    _PAULI_TO_UINT8_CODE = {"X": 1, "Y": 3, "Z": 5}

    def _get_clean_known_states(
        self, pauli: Literal["X", "Y", "Z"], *, circuit_time: int | None = None
    ) -> tuple[Bool1DArray, Bool1DArray]:
        """given circuit locations, return the state of the target under noiseless execution."""
        if self._known_states is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        if self._known_states_uint8 is not None:
            row = self._known_states_uint8[circuit_time]
            code = self._PAULI_TO_UINT8_CODE[pauli]
            return (row == code, row == (code + 1))
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
        """returns true if all targets are in a known state of the given pauli."""
        if self._known_states is None:
            raise ValueError(
                "Can't access known_states when initialized with compute_known_states=False."
            )
        if circuit_time is None:
            circuit_time = self.get_current_circuit_time()
        if self._known_states_uint8 is not None:
            row = self._known_states_uint8[circuit_time]
            code = self._PAULI_TO_UINT8_CODE[pauli]
            return all(row[int(t)] == code or row[int(t)] == (code + 1) for t in targets)
        in_plus, in_minus = self._get_clean_known_states(
            pauli=pauli, circuit_time=circuit_time
        )
        in_known = np.logical_or(in_plus, in_minus)
        return all(in_known[t] for t in targets)

    def _get_current_known_state_masks(
        self, pauli: Literal["X", "Y", "Z"], *, circuit_time: int | None = None
    ) -> tuple[Bool2DArray, Bool2DArray]:
        """given circuit locations, return masks indicating the qubits known states."""
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

