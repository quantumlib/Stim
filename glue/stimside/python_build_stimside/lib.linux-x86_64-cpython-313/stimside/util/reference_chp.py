from __future__ import annotations

import dataclasses
from typing import Literal, Sequence

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]


_GATE_TABLEAU_CACHE: dict[str, tuple[stim.Tableau, stim.Tableau]] = {}
RawFactor = (
    tuple[Literal["P1", "P2"], tuple[int, ...], tuple[int, ...], int]
    | tuple[
        Literal["PCP"],
        tuple[int, ...],
        tuple[int, ...],
        int,
        tuple[int, ...],
        tuple[int, ...],
        int,
    ]
)

_GATE_MRQF_FACTORS_CACHE: dict[str, tuple[RawFactor, ...]] = {}


def get_cached_gate_tableau(gate_name: str) -> tuple[stim.Tableau, stim.Tableau]:
    """Return cached (forward_tableau, inverse_tableau) for a named Stim gate."""
    cached = _GATE_TABLEAU_CACHE.get(gate_name)
    if cached is not None:
        return cached
    fwd = stim.Tableau.from_named_gate(gate_name)
    inv = fwd.inverse()
    _GATE_TABLEAU_CACHE[gate_name] = (fwd, inv)
    return fwd, inv


def _bools_to_uint64_words(bits: NDArray[np.bool_], n_words: int) -> NDArray[np.uint64]:
    """Pack a 1D boolean array into a 1D uint64 word array (little-endian bit order)."""
    packed_u8 = np.packbits(bits, bitorder="little")
    padded = np.zeros(n_words * 8, dtype=np.uint8)
    padded[: len(packed_u8)] = packed_u8
    return padded.view(np.uint64)


# Elementary local decompositions for E = U_gate^dagger in application order (rightmost factor first).
# Entries are either:
# - ("P1" | "P2", local_x_tuple, local_z_tuple, sign_pm1)
# - ("PCP", local_x1, local_z1, sign1, local_x2, local_z2, sign2) for (I + P1 + P2 - P1 P2) / 2
_EXPLICIT_INVERSE_GATE_FACTORS: dict[str, tuple[RawFactor, ...]] = {
    "I": (),
    "II": (),
    "X": (("P1", (1,), (0,), 1),),
    "Y": (("P1", (1,), (1,), 1),),
    "Z": (("P1", (0,), (1,), 1),),
    # S^dagger = R(+Z); S = R(-Z)
    "S": (("P2", (0,), (1,), 1),),
    "S_DAG": (("P2", (0,), (1,), -1),),
    # H^dagger = H = R(Y) X (apply X first, then R(Y))
    "H": (("P1", (1,), (0,), 1), ("P2", (1,), (1,), 1)),
    "H_XZ": (("P1", (1,), (0,), 1), ("P2", (1,), (1,), 1)),
    # H_XY^dagger = H_XY = (X + Y)/sqrt(2) = R(-Z) X
    "H_XY": (("P1", (1,), (0,), 1), ("P2", (0,), (1,), -1)),
    # H_YZ^dagger = H_YZ = (Y + Z)/sqrt(2) = R(+X) Z
    "H_YZ": (("P1", (0,), (1,), 1), ("P2", (1,), (0,), 1)),
    # SQRT_X^dagger = R(+X); SQRT_X = R(-X)
    "SQRT_X": (("P2", (1,), (0,), 1),),
    "SQRT_X_DAG": (("P2", (1,), (0,), -1),),
    # SQRT_Y^dagger = R(+Y); SQRT_Y = R(-Y)
    "SQRT_Y": (("P2", (1,), (1,), 1),),
    "SQRT_Y_DAG": (("P2", (1,), (1,), -1),),
    # Period-3 single-qubit rotations: C_XYZ^dagger = R(X) R(Z), C_ZYX^dagger = R(-Z) R(-X)
    "C_XYZ": (("P2", (0,), (1,), 1), ("P2", (1,), (0,), 1)),
    "C_ZYX": (("P2", (1,), (0,), -1), ("P2", (0,), (1,), -1)),
    # Two-qubit quarter-turn rotations: SQRT_PP^dagger = R(+P_0 P_1)
    "SQRT_XX": (("P2", (1, 1), (0, 0), 1),),
    "SQRT_XX_DAG": (("P2", (1, 1), (0, 0), -1),),
    "SQRT_YY": (("P2", (1, 1), (1, 1), 1),),
    "SQRT_YY_DAG": (("P2", (1, 1), (1, 1), -1),),
    "SQRT_ZZ": (("P2", (0, 0), (1, 1), 1),),
    "SQRT_ZZ_DAG": (("P2", (0, 0), (1, 1), -1),),
    # Controlled-Pauli gates via direct 2-column rank-2 injection: U^dagger = U = (I + P_1 + P_2 - P_1 P_2) / 2
    "CZ": (("PCP", (0, 0), (1, 0), 1, (0, 0), (0, 1), 1),),
    "ZCZ": (("PCP", (0, 0), (1, 0), 1, (0, 0), (0, 1), 1),),
    "CX": (("PCP", (0, 0), (1, 0), 1, (0, 1), (0, 0), 1),),
    "CNOT": (("PCP", (0, 0), (1, 0), 1, (0, 1), (0, 0), 1),),
    "ZCX": (("PCP", (0, 0), (1, 0), 1, (0, 1), (0, 0), 1),),
    "CY": (("PCP", (0, 0), (1, 0), 1, (0, 1), (0, 1), 1),),
    "ZCY": (("PCP", (0, 0), (1, 0), 1, (0, 1), (0, 1), 1),),
    "XCX": (("PCP", (1, 0), (0, 0), 1, (0, 1), (0, 0), 1),),
    "XCY": (("PCP", (1, 0), (0, 0), 1, (0, 1), (0, 1), 1),),
    "XCZ": (("PCP", (1, 0), (0, 0), 1, (0, 0), (0, 1), 1),),
    "YCX": (("PCP", (1, 0), (1, 0), 1, (0, 1), (0, 0), 1),),
    "YCY": (("PCP", (1, 0), (1, 0), 1, (0, 1), (0, 1), 1),),
    "YCZ": (("PCP", (1, 0), (1, 0), 1, (0, 0), (0, 1), 1),),
    # SWAP^dagger = SWAP = R(-X_0 X_1) R(-Y_0 Y_1) R(-Z_0 Z_1)
    "SWAP": (
        ("P2", (0, 0), (1, 1), -1),
        ("P2", (1, 1), (1, 1), -1),
        ("P2", (1, 1), (0, 0), -1),
    ),
    # ISWAP^dagger = R(-Y_0 Y_1) R(-X_0 X_1); ISWAP_DAG^dagger = R(+Y_0 Y_1) R(+X_0 X_1)
    "ISWAP": (
        ("P2", (1, 1), (0, 0), -1),
        ("P2", (1, 1), (1, 1), -1),
    ),
    "ISWAP_DAG": (
        ("P2", (1, 1), (0, 0), 1),
        ("P2", (1, 1), (1, 1), 1),
    ),
}


# Forward elementary decompositions used when decomposing general inverse tableaus via to_circuit().
_FORWARD_ELEMENTARY_FACTORS: dict[str, tuple[RawFactor, ...]] = {
    "X": (("P1", (1,), (0,), 1),),
    "Y": (("P1", (1,), (1,), 1),),
    "Z": (("P1", (0,), (1,), 1),),
    "H": (("P1", (1,), (0,), 1), ("P2", (1,), (1,), 1)),
    "S": (("P2", (0,), (1,), -1),),
    "S_DAG": (("P2", (0,), (1,), 1),),
    "CX": _EXPLICIT_INVERSE_GATE_FACTORS["CX"],
    "CZ": _EXPLICIT_INVERSE_GATE_FACTORS["CZ"],
}


def get_inverse_clifford_mrqf_factors(
    gate_name: str,
) -> tuple[RawFactor, ...]:
    """Return the ordered tuple of MRQF primitive factors for E = U_gate^dagger."""
    cached = _GATE_MRQF_FACTORS_CACHE.get(gate_name)
    if cached is not None:
        return cached
    if gate_name in _EXPLICIT_INVERSE_GATE_FACTORS:
        res = _EXPLICIT_INVERSE_GATE_FACTORS[gate_name]
        _GATE_MRQF_FACTORS_CACHE[gate_name] = res
        return res

    _, inv_tab = get_cached_gate_tableau(gate_name)
    arity = len(inv_tab)
    circ = inv_tab.to_circuit(method="elimination")
    factors: list[RawFactor] = []
    for inst in circ:
        sub_factors = _FORWARD_ELEMENTARY_FACTORS[inst.name]
        targets = [t.value for t in inst.targets_copy()]
        step = len(sub_factors[0][1])
        for i in range(0, len(targets), step):
            grp = targets[i : i + step]
            for f in sub_factors:
                if f[0] == "PCP":
                    _, lx1, lz1, sgn1, lx2, lz2, sgn2 = f
                    fx1 = [0] * arity
                    fz1 = [0] * arity
                    fx2 = [0] * arity
                    fz2 = [0] * arity
                    for r_local, q_local in enumerate(grp):
                        fx1[q_local] = lx1[r_local]
                        fz1[q_local] = lz1[r_local]
                        fx2[q_local] = lx2[r_local]
                        fz2[q_local] = lz2[r_local]
                    factors.append(
                        (
                            "PCP",
                            tuple(fx1),
                            tuple(fz1),
                            sgn1,
                            tuple(fx2),
                            tuple(fz2),
                            sgn2,
                        )
                    )
                else:
                    kind, lx, lz, sgn = f
                    full_x = [0] * arity
                    full_z = [0] * arity
                    for r_local, q_local in enumerate(grp):
                        full_x[q_local] = lx[r_local]
                        full_z[q_local] = lz[r_local]
                    factors.append((kind, tuple(full_x), tuple(full_z), sgn))
    res = tuple(factors)
    _GATE_MRQF_FACTORS_CACHE[gate_name] = res
    return res


@dataclasses.dataclass
class CompiledFaultFactor:
    """Single compiled P1 (Pauli), P2 (quarter-turn), or PCP (controlled-Pauli) primitive in reference coordinates B_t."""

    kind: Literal["P1", "P2", "PCP"]
    local_x: NDArray[np.bool_]  # shape (k,)
    local_z: NDArray[np.bool_]  # shape (k,)
    a_words: NDArray[np.uint64]  # shape (n_words,)
    b_words: NDArray[np.uint64]  # shape (n_words,)
    kappa: int  # in {0, 1, 2, 3}
    local_x2: NDArray[np.bool_] | None = None
    local_z2: NDArray[np.bool_] | None = None
    a2_words: NDArray[np.uint64] | None = None
    b2_words: NDArray[np.uint64] | None = None
    kappa2: int = 0


@dataclasses.dataclass
class InverseCliffordCocycle:
    """Compiled MRQF decomposition of E = U_t^dagger at step t+1 on target group K."""

    targets: tuple[int, ...]
    factors: tuple[CompiledFaultFactor, ...]


@dataclasses.dataclass
class DetMeasMeta:
    """Metadata for a deterministic Z_q measurement (a = 0) in the noiseless reference circuit."""

    is_random: Literal[False]
    q: int
    m_ref: int
    beta_Z_words: NDArray[np.uint64]
    reset_flip_ref: bool = False


@dataclasses.dataclass
class RandMeasMeta:
    """Metadata for a random Z_q measurement (a != 0) in the noiseless reference circuit."""

    is_random: Literal[True]
    q: int
    m_ref: int
    p: int
    a_words: NDArray[np.uint64]
    b_rot_words: NDArray[np.uint64]
    e_p_words: NDArray[np.uint64]
    kappa_rot: int
    G_x_indices: NDArray[np.intp]
    G_z_indices: NDArray[np.intp]
    reset_flip_ref: bool = False


PackedSymplecticTables = tuple[
    NDArray[np.uint64],
    NDArray[np.uint64],
    NDArray[np.uint64],
    NDArray[np.uint64],
    NDArray[np.uint8],
    NDArray[np.uint8],
]


def _packed_tables_to_stim_tableau(
    tables: PackedSymplecticTables, num_qubits: int
) -> stim.Tableau:
    x2x_w, x2z_w, z2x_w, z2z_w, x_kap, z_kap = tables
    n_bytes = (num_qubits + 7) // 8
    x2x = np.unpackbits(
        x2x_w.view(np.uint8)[:, :n_bytes], axis=1, bitorder="little"
    )[:, :num_qubits].astype(np.bool_)
    x2z = np.unpackbits(
        x2z_w.view(np.uint8)[:, :n_bytes], axis=1, bitorder="little"
    )[:, :num_qubits].astype(np.bool_)
    z2x = np.unpackbits(
        z2x_w.view(np.uint8)[:, :n_bytes], axis=1, bitorder="little"
    )[:, :num_qubits].astype(np.bool_)
    z2z = np.unpackbits(
        z2z_w.view(np.uint8)[:, :n_bytes], axis=1, bitorder="little"
    )[:, :num_qubits].astype(np.bool_)
    x_pop = np.bitwise_count(x2x_w & x2z_w).sum(axis=1, dtype=np.int32) & 3
    z_pop = np.bitwise_count(z2x_w & z2z_w).sum(axis=1, dtype=np.int32) & 3
    x_signs = (((x_kap.astype(np.int32) - x_pop) & 3) == 2).astype(np.bool_)
    z_signs = (((z_kap.astype(np.int32) - z_pop) & 3) == 2).astype(np.bool_)
    return stim.Tableau.from_numpy(
        x2x=x2x, x2z=x2z, z2x=z2x, z2z=z2z, x_signs=x_signs, z_signs=z_signs
    )


def _stim_tableau_to_packed_tables(
    tab: stim.Tableau, num_qubits: int, n_words: int
) -> PackedSymplecticTables:
    x2x, x2z, z2x, z2z, x_signs, z_signs = tab.to_numpy(bit_packed=True)
    n_bytes = z2x.shape[1]

    def _pad_u64(arr_u8: NDArray[np.uint8]) -> NDArray[np.uint64]:
        out = np.zeros((num_qubits, n_words * 8), dtype=np.uint8)
        out[:, :n_bytes] = arr_u8
        return out.view(np.uint64)

    x2x_w, x2z_w, z2x_w, z2z_w = _pad_u64(x2x), _pad_u64(x2z), _pad_u64(z2x), _pad_u64(z2z)
    x_s = np.unpackbits(x_signs, bitorder="little")[:num_qubits].astype(np.uint8)
    z_s = np.unpackbits(z_signs, bitorder="little")[:num_qubits].astype(np.uint8)
    x_kap = ((np.bitwise_count(x2x_w & x2z_w).sum(axis=1) + (x_s << 1)) & 3).astype(np.uint8)
    z_kap = ((np.bitwise_count(z2x_w & z2z_w).sum(axis=1) + (z_s << 1)) & 3).astype(np.uint8)
    return (x2x_w, x2z_w, z2x_w, z2z_w, x_kap, z_kap)


@dataclasses.dataclass
class UnitaryStepMeta:
    kind: Literal["unitary"]
    op: stim.CircuitInstruction
    gate_name: str
    num_qubits: int
    target_groups: list[tuple[int, ...]]
    _group_ops: list[stim.CircuitInstruction] | None = None
    _group_inv_tableaus: list[stim.Tableau] | None = None
    _group_packed_words: list[PackedSymplecticTables] | None = None
    _inv_tableau_after: stim.Tableau | None = None
    _prev_step: StepMeta | None = None
    _cocycle_cache: dict[int, InverseCliffordCocycle] = dataclasses.field(default_factory=dict)
    are_groups_disjoint: bool = True
    _packed_words: PackedSymplecticTables | None = None
    _evict_tables: tuple[NDArray[np.uint8], NDArray[np.uint8]] | None = None

    @property
    def inv_tableau_after(self) -> stim.Tableau:
        if self._inv_tableau_after is None:
            if self._packed_words is not None:
                self._inv_tableau_after = _packed_tables_to_stim_tableau(self._packed_words, self.num_qubits)
            else:
                base_tab = (stim.Tableau(self.num_qubits) if self._prev_step is None else self._prev_step.inv_tableau_after.copy())
                if self.gate_name not in ("I", "II", "I_ERROR", "II_ERROR"):
                    _, inv_gate = get_cached_gate_tableau(self.gate_name)
                    for grp in self.target_groups:
                        base_tab.prepend(inv_gate, list(grp))
                self._inv_tableau_after = base_tab
        return self._inv_tableau_after

    def get_evict_tables(self) -> tuple[NDArray[np.uint8], NDArray[np.uint8]]:
        if self._evict_tables is None:
            if self._packed_words is not None:
                n_bytes = (self.num_qubits + 7) // 8
                self._evict_tables = (self._packed_words[1].view(np.uint8)[:, :n_bytes], self._packed_words[3].view(np.uint8)[:, :n_bytes])
            else:
                _, x2z, _, z2z, _, _ = self.inv_tableau_after.to_numpy(bit_packed=True)
                self._evict_tables = (x2z, z2z)
        return self._evict_tables

    def get_evict_words(
        self, n_words: int
    ) -> tuple[NDArray[np.uint64], NDArray[np.uint64]]:
        pw = self._get_packed_words(n_words)
        return pw[1], pw[3]

    get_evict_x2z_z2z = get_evict_tables

    @property
    def group_ops(self) -> list[stim.CircuitInstruction]:
        if self._group_ops is None:
            self._group_ops = [
                stim.CircuitInstruction(self.gate_name, list(grp))
                for grp in self.target_groups
            ]
        return self._group_ops

    @property
    def group_inv_tableaus(self) -> list[stim.Tableau]:
        if self._group_inv_tableaus is None:
            if self._group_packed_words is not None:
                self._group_inv_tableaus = [
                    _packed_tables_to_stim_tableau(pw, self.num_qubits)
                    for pw in self._group_packed_words
                ]
            else:
                self._group_inv_tableaus = [self.inv_tableau_after] * len(
                    self.target_groups
                )
        return self._group_inv_tableaus

    def _get_packed_words(self, n_words: int) -> PackedSymplecticTables:
        if self._packed_words is None:
            self._packed_words = _stim_tableau_to_packed_tables(
                self.inv_tableau_after, self.num_qubits, n_words
            )
        return self._packed_words

    def get_inverse_cocycle(
        self, g_idx: int, num_qubits: int, n_words: int
    ) -> InverseCliffordCocycle:
        """Lazily compute and cache the compiled MRQF fault factors for E = U_t^dagger on group g_idx."""
        cached = self._cocycle_cache.get(g_idx)
        if cached is not None:
            return cached

        target_group = self.target_groups[g_idx]
        raw_factors = get_inverse_clifford_mrqf_factors(self.gate_name)
        if self.are_groups_disjoint:
            packed_tables: PackedSymplecticTables | None = self._get_packed_words(n_words)
        elif self._group_packed_words is not None:
            packed_tables = self._group_packed_words[g_idx]
        else:
            packed_tables = None
        inv_tab = None if packed_tables is not None else self.group_inv_tableaus[g_idx]

        single_q_cache: dict[
            tuple[int, int, int],
            tuple[NDArray[np.uint64], NDArray[np.uint64], int],
        ] = {}

        def _compile_pauli_tuple(
            lx: tuple[int, ...], lz: tuple[int, ...], sgn: int
        ) -> tuple[NDArray[np.uint64], NDArray[np.uint64], int]:
            active_terms: list[
                tuple[NDArray[np.uint64], NDArray[np.uint64], int]
            ] = []
            for r in range(len(target_group)):
                bx, bz = lx[r], lz[r]
                if bx == 0 and bz == 0:
                    continue
                key = (r, bx, bz)
                term = single_q_cache.get(key)
                if term is None:
                    q_r = target_group[r]
                    if packed_tables is not None:
                        x2x_w, x2z_w, z2x_w, z2z_w, x_kappa, z_kappa = (
                            packed_tables
                        )
                        if bx == 1 and bz == 0:
                            term = (
                                x2x_w[q_r],
                                x2z_w[q_r],
                                int(x_kappa[q_r]),
                            )
                        elif bx == 0 and bz == 1:
                            term = (
                                z2x_w[q_r],
                                z2z_w[q_r],
                                int(z_kappa[q_r]),
                            )
                        else:
                            aw_y = x2x_w[q_r] ^ z2x_w[q_r]
                            bw_y = x2z_w[q_r] ^ z2z_w[q_r]
                            dp = (
                                int(
                                    np.bitwise_count(
                                        x2z_w[q_r] & z2x_w[q_r]
                                    ).sum()
                                )
                                & 1
                            )
                            kap_y = (
                                1
                                + int(x_kappa[q_r])
                                + int(z_kappa[q_r])
                                + (dp << 1)
                            ) & 3
                            term = (aw_y, bw_y, kap_y)
                    else:
                        assert inv_tab is not None
                        if bx == 1 and bz == 0:
                            d_single = inv_tab.x_output(q_r)
                        elif bx == 0 and bz == 1:
                            d_single = inv_tab.z_output(q_r)
                        else:
                            d_single = inv_tab.y_output(q_r)
                        a_s, b_s = d_single.to_numpy()
                        aw_s = _bools_to_uint64_words(a_s, n_words)
                        bw_s = _bools_to_uint64_words(b_s, n_words)
                        kap_s = (
                            int(np.bitwise_count(aw_s & bw_s).sum())
                            + (2 if d_single.sign == -1 else 0)
                        ) & 3
                        term = (aw_s, bw_s, kap_s)
                    single_q_cache[key] = term
                active_terms.append(term)

            if len(active_terms) == 1:
                a_w, b_w, kap = active_terms[0]
                a_w = a_w.copy()
                b_w = b_w.copy()
            else:
                aw0, bw0, kap0 = active_terms[0]
                aw1, bw1, kap1 = active_terms[1]
                dp = int(np.bitwise_count(bw0 & aw1).sum()) & 1
                a_w = aw0 ^ aw1
                b_w = bw0 ^ bw1
                kap = (kap0 + kap1 + (dp << 1)) & 3

            if sgn == -1:
                kap = (kap + 2) & 3
            return a_w, b_w, kap

        compiled_factors: list[CompiledFaultFactor] = []
        for f in raw_factors:
            if f[0] == "PCP":
                _, lx1, lz1, sgn1, lx2, lz2, sgn2 = f
                aw1, bw1, kap1 = _compile_pauli_tuple(lx1, lz1, sgn1)
                aw2, bw2, kap2 = _compile_pauli_tuple(lx2, lz2, sgn2)
                compiled_factors.append(
                    CompiledFaultFactor(
                        kind="PCP",
                        local_x=np.asarray(lx1, dtype=np.bool_),
                        local_z=np.asarray(lz1, dtype=np.bool_),
                        a_words=aw1,
                        b_words=bw1,
                        kappa=kap1,
                        local_x2=np.asarray(lx2, dtype=np.bool_),
                        local_z2=np.asarray(lz2, dtype=np.bool_),
                        a2_words=aw2,
                        b2_words=bw2,
                        kappa2=kap2,
                    )
                )
            else:
                kind, lx, lz, sgn = f
                a_w, b_w, kap = _compile_pauli_tuple(lx, lz, sgn)
                compiled_factors.append(
                    CompiledFaultFactor(
                        kind=kind,
                        local_x=np.asarray(lx, dtype=np.bool_),
                        local_z=np.asarray(lz, dtype=np.bool_),
                        a_words=a_w,
                        b_words=b_w,
                        kappa=kap,
                    )
                )

        cocycle = InverseCliffordCocycle(
            targets=target_group,
            factors=tuple(compiled_factors),
        )
        self._cocycle_cache[g_idx] = cocycle
        return cocycle


GateStepMeta = UnitaryStepMeta


@dataclasses.dataclass
class MeasSubStepMeta:
    """Single measurement or reset substep within a MeasStepMeta instruction."""

    q: int
    invert_result: bool
    pre_ops: list[stim.CircuitInstruction]
    post_ops: list[stim.CircuitInstruction]
    meta: DetMeasMeta | RandMeasMeta


@dataclasses.dataclass
class MeasStepMeta:
    """Compiled step descriptor for a measurement or reset instruction."""

    kind: Literal["meas"]
    op: stim.CircuitInstruction
    gate_name: str
    produces_measurements: bool
    is_reset: bool
    substeps: list[MeasSubStepMeta]
    _inv_tableau_after: stim.Tableau | None = None
    _packed_words: PackedSymplecticTables | None = None
    num_qubits: int = 0
    _det_batch_cache: (
        tuple[NDArray[np.intp], NDArray[np.bool_], NDArray[np.uint64]] | None
    ) = None
    _evict_x2z_z2z_cache: tuple[NDArray[np.uint8], NDArray[np.uint8]] | None = (
        None
    )

    @property
    def inv_tableau_after(self) -> stim.Tableau:
        if self._inv_tableau_after is None:
            assert self._packed_words is not None
            self._inv_tableau_after = _packed_tables_to_stim_tableau(
                self._packed_words, self.num_qubits
            )
        return self._inv_tableau_after

    def _get_packed_words(self, n_words: int) -> PackedSymplecticTables:
        if self._packed_words is None:
            self._packed_words = _stim_tableau_to_packed_tables(
                self.inv_tableau_after, self.num_qubits, n_words
            )
        return self._packed_words

    def get_evict_x2z_z2z(self) -> tuple[NDArray[np.uint8], NDArray[np.uint8]]:
        if self._evict_x2z_z2z_cache is None:
            if self._packed_words is not None:
                n_bytes = (self.num_qubits + 7) // 8
                x2z = self._packed_words[1].view(np.uint8)[:, :n_bytes]
                z2z = self._packed_words[3].view(np.uint8)[:, :n_bytes]
                self._evict_x2z_z2z_cache = (x2z, z2z)
            else:
                _, x2z, _, z2z, _, _ = self.inv_tableau_after.to_numpy(
                    bit_packed=True
                )
                self._evict_x2z_z2z_cache = (x2z, z2z)
        return self._evict_x2z_z2z_cache

    def get_evict_words(
        self, n_words: int
    ) -> tuple[NDArray[np.uint64], NDArray[np.uint64]]:
        pw = self._get_packed_words(n_words)
        return pw[1], pw[3]

    def get_det_batch_arrays(
        self,
    ) -> tuple[NDArray[np.intp], NDArray[np.bool_], NDArray[np.uint64]]:
        if self._det_batch_cache is None:
            num_sub = len(self.substeps)
            q_arr = np.fromiter(
                (s.meta.q for s in self.substeps), dtype=np.intp, count=num_sub
            )
            m_ref_arr = np.fromiter(
                (bool(s.meta.m_ref) for s in self.substeps),
                dtype=np.bool_,
                count=num_sub,
            )
            b_sub_words = np.vstack(
                [s.meta.beta_Z_words for s in self.substeps]  # type: ignore[union-attr]
            )
            self._det_batch_cache = (q_arr, m_ref_arr, b_sub_words)
        return self._det_batch_cache


@dataclasses.dataclass
class NoopStepMeta:
    """Compiled step descriptor for noise/annotation instructions that leave the reference state invariant."""

    kind: Literal["noop"]
    op: stim.CircuitInstruction
    _inv_tableau_after: stim.Tableau | None = None
    _packed_words: PackedSymplecticTables | None = None
    _prev_step: StepMeta | None = None
    num_qubits: int = 0
    _evict_x2z_z2z_cache: tuple[NDArray[np.uint8], NDArray[np.uint8]] | None = (
        None
    )

    @property
    def inv_tableau_after(self) -> stim.Tableau:
        if self._inv_tableau_after is None:
            if self._packed_words is not None:
                self._inv_tableau_after = _packed_tables_to_stim_tableau(
                    self._packed_words, self.num_qubits
                )
            elif self._prev_step is None:
                self._inv_tableau_after = stim.Tableau(self.num_qubits)
            else:
                self._inv_tableau_after = self._prev_step.inv_tableau_after
        return self._inv_tableau_after

    def _get_packed_words(self, n_words: int) -> PackedSymplecticTables:
        if self._packed_words is not None:
            return self._packed_words
        if self._prev_step is not None:
            self._packed_words = self._prev_step._get_packed_words(n_words)
            return self._packed_words
        self._packed_words = _stim_tableau_to_packed_tables(
            self.inv_tableau_after, self.num_qubits, n_words
        )
        return self._packed_words

    def get_evict_x2z_z2z(self) -> tuple[NDArray[np.uint8], NDArray[np.uint8]]:
        if self._evict_x2z_z2z_cache is None:
            if self._packed_words is not None:
                n_bytes = (self.num_qubits + 7) // 8
                x2z = self._packed_words[1].view(np.uint8)[:, :n_bytes]
                z2z = self._packed_words[3].view(np.uint8)[:, :n_bytes]
                self._evict_x2z_z2z_cache = (x2z, z2z)
            else:
                _, x2z, _, z2z, _, _ = self.inv_tableau_after.to_numpy(
                    bit_packed=True
                )
                self._evict_x2z_z2z_cache = (x2z, z2z)
        return self._evict_x2z_z2z_cache

    def get_evict_words(
        self, n_words: int
    ) -> tuple[NDArray[np.uint64], NDArray[np.uint64]]:
        pw = self._get_packed_words(n_words)
        return pw[1], pw[3]


StepMeta = UnitaryStepMeta | MeasStepMeta | NoopStepMeta


class ReferenceCHPTableau:
    """Symplectic stabilizer-destabilizer reference engine operating directly on packed uint64 coordinate arrays without tableau conjugation or inversion."""

    def __init__(self, num_qubits: int) -> None:
        self.num_qubits = num_qubits
        self.n_words = max(1, (num_qubits + 63) // 64)
        self.x2x = np.zeros((num_qubits, self.n_words), dtype=np.uint64)
        self.x2z = np.zeros((num_qubits, self.n_words), dtype=np.uint64)
        self.z2x = np.zeros((num_qubits, self.n_words), dtype=np.uint64)
        self.z2z = np.zeros((num_qubits, self.n_words), dtype=np.uint64)
        for q in range(num_qubits):
            self.x2x[q, q >> 6] = np.uint64(1 << (q & 63))
            self.z2z[q, q >> 6] = np.uint64(1 << (q & 63))
        self.x_kap = np.zeros(num_qubits, dtype=np.uint8)
        self.z_kap = np.zeros(num_qubits, dtype=np.uint8)
        self._inv_dirty = False

    def snapshot_packed(self) -> PackedSymplecticTables:
        return (
            self.x2x.copy(),
            self.x2z.copy(),
            self.z2x.copy(),
            self.z2z.copy(),
            self.x_kap.copy(),
            self.z_kap.copy(),
        )

    @property
    def inv_tableau(self) -> stim.Tableau:
        return _packed_tables_to_stim_tableau(
            (self.x2x, self.x2z, self.z2x, self.z2z, self.x_kap, self.z_kap),
            self.num_qubits,
        )

    @inv_tableau.setter
    def inv_tableau(self, val: stim.Tableau) -> None:
        (
            self.x2x,
            self.x2z,
            self.z2x,
            self.z2z,
            self.x_kap,
            self.z_kap,
        ) = _stim_tableau_to_packed_tables(val, self.num_qubits, self.n_words)
        self._inv_dirty = True

    @property
    def tableau(self) -> stim.Tableau:
        return self.inv_tableau.inverse()

    @tableau.setter
    def tableau(self, val: stim.Tableau) -> None:
        self.inv_tableau = val.inverse()

    def do_unitary(self, gate_name: str, targets: Sequence[int]) -> None:
        """Update C_t <- U C_t directly on packed uint64 coordinate arrays without stim.Tableau."""
        if gate_name in ("I", "II", "I_ERROR", "II_ERROR") or not targets:
            return
        self._inv_dirty = True
        q_arr = np.asarray(targets, dtype=np.intp)

        if gate_name in ("H", "H_XZ"):
            self.x2x[q_arr], self.z2x[q_arr] = (
                self.z2x[q_arr].copy(),
                self.x2x[q_arr].copy(),
            )
            self.x2z[q_arr], self.z2z[q_arr] = (
                self.z2z[q_arr].copy(),
                self.x2z[q_arr].copy(),
            )
            self.x_kap[q_arr], self.z_kap[q_arr] = (
                self.z_kap[q_arr].copy(),
                self.x_kap[q_arr].copy(),
            )
            return

        if gate_name in ("CZ", "ZCZ"):
            q0 = q_arr[0::2]
            q1 = q_arr[1::2]
            dp0 = np.bitwise_count(self.x2z[q0] & self.z2x[q1]).sum(axis=1) & 1
            dp1 = np.bitwise_count(self.x2z[q1] & self.z2x[q0]).sum(axis=1) & 1
            self.x2x[q0] ^= self.z2x[q1]
            self.x2z[q0] ^= self.z2z[q1]
            self.x_kap[q0] = (
                self.x_kap[q0] + self.z_kap[q1] + (dp0 << 1)
            ) & 3
            self.x2x[q1] ^= self.z2x[q0]
            self.x2z[q1] ^= self.z2z[q0]
            self.x_kap[q1] = (
                self.x_kap[q1] + self.z_kap[q0] + (dp1 << 1)
            ) & 3
            return

        if gate_name in ("CX", "CNOT", "ZCX"):
            c = q_arr[0::2]
            t = q_arr[1::2]
            dpx = np.bitwise_count(self.x2z[c] & self.x2x[t]).sum(axis=1) & 1
            dpz = np.bitwise_count(self.z2z[t] & self.z2x[c]).sum(axis=1) & 1
            self.x2x[c] ^= self.x2x[t]
            self.x2z[c] ^= self.x2z[t]
            self.x_kap[c] = (self.x_kap[c] + self.x_kap[t] + (dpx << 1)) & 3
            self.z2x[t] ^= self.z2x[c]
            self.z2z[t] ^= self.z2z[c]
            self.z_kap[t] = (self.z_kap[t] + self.z_kap[c] + (dpz << 1)) & 3
            return

        if gate_name == "X":
            self.z_kap[q_arr] = (self.z_kap[q_arr] + 2) & 3
            return
        if gate_name == "Z":
            self.x_kap[q_arr] = (self.x_kap[q_arr] + 2) & 3
            return
        if gate_name == "Y":
            self.x_kap[q_arr] = (self.x_kap[q_arr] + 2) & 3
            self.z_kap[q_arr] = (self.z_kap[q_arr] + 2) & 3
            return

        if gate_name in ("S", "S_DAG"):
            phase_shift = 3 if gate_name == "S" else 1
            dp = (
                np.bitwise_count(self.x2z[q_arr] & self.z2x[q_arr]).sum(axis=1)
                & 1
            )
            self.x2x[q_arr] ^= self.z2x[q_arr]
            self.x2z[q_arr] ^= self.z2z[q_arr]
            self.x_kap[q_arr] = (
                phase_shift
                + self.x_kap[q_arr]
                + self.z_kap[q_arr]
                + (dp << 1)
            ) & 3
            return

        if gate_name == "SWAP":
            q0 = q_arr[0::2]
            q1 = q_arr[1::2]
            self.x2x[q0], self.x2x[q1] = (
                self.x2x[q1].copy(),
                self.x2x[q0].copy(),
            )
            self.x2z[q0], self.x2z[q1] = (
                self.x2z[q1].copy(),
                self.x2z[q0].copy(),
            )
            self.z2x[q0], self.z2x[q1] = (
                self.z2x[q1].copy(),
                self.z2x[q0].copy(),
            )
            self.z2z[q0], self.z2z[q1] = (
                self.z2z[q1].copy(),
                self.z2z[q0].copy(),
            )
            self.x_kap[q0], self.x_kap[q1] = (
                self.x_kap[q1].copy(),
                self.x_kap[q0].copy(),
            )
            self.z_kap[q0], self.z_kap[q1] = (
                self.z_kap[q1].copy(),
                self.z_kap[q0].copy(),
            )
            return

        # General local fallback for any 1- or 2-qubit named gate via its 1x1 or 2x2 inverse tableau
        _, inv_gate = get_cached_gate_tableau(gate_name)
        arity = len(inv_gate)
        for i in range(0, len(targets), arity):
            grp = targets[i : i + arity]
            old_x = [
                (self.x2x[q].copy(), self.x2z[q].copy(), int(self.x_kap[q]))
                for q in grp
            ]
            old_z = [
                (self.z2x[q].copy(), self.z2z[q].copy(), int(self.z_kap[q]))
                for q in grp
            ]
            for r_loc, q_phys in enumerate(grp):
                for is_z_out, p_str in (
                    (False, inv_gate.x_output(r_loc)),
                    (True, inv_gate.z_output(r_loc)),
                ):
                    xs_l, zs_l = p_str.to_numpy()
                    aw_acc = np.zeros(self.n_words, dtype=np.uint64)
                    bw_acc = np.zeros(self.n_words, dtype=np.uint64)
                    kap_acc = 2 if p_str.sign == -1 else 0
                    for k_loc in range(arity):
                        bx, bz = int(xs_l[k_loc]), int(zs_l[k_loc])
                        if bx == 0 and bz == 0:
                            continue
                        if bx == 1 and bz == 0:
                            aw_t, bw_t, kap_t = old_x[k_loc]
                        elif bx == 0 and bz == 1:
                            aw_t, bw_t, kap_t = old_z[k_loc]
                        else:
                            ax, bx_w, kx = old_x[k_loc]
                            az, bz_w, kz = old_z[k_loc]
                            dp = int(np.bitwise_count(bx_w & az).sum()) & 1
                            aw_t = ax ^ az
                            bw_t = bx_w ^ bz_w
                            kap_t = (1 + kx + kz + (dp << 1)) & 3
                        dp_acc = int(np.bitwise_count(bw_acc & aw_t).sum()) & 1
                        aw_acc ^= aw_t
                        bw_acc ^= bw_t
                        kap_acc = (kap_acc + kap_t + (dp_acc << 1)) & 3
                    if is_z_out:
                        self.z2x[q_phys] = aw_acc
                        self.z2z[q_phys] = bw_acc
                        self.z_kap[q_phys] = kap_acc
                    else:
                        self.x2x[q_phys] = aw_acc
                        self.x2z[q_phys] = bw_acc
                        self.x_kap[q_phys] = kap_acc

    def measure_z(
        self, q: int, *, is_reset: bool = False, m_ref_override: int = 0
    ) -> DetMeasMeta | RandMeasMeta:
        """Execute a reference Z_q measurement/reset via direct rank-1 symplectic transvection."""
        aw = self.z2x[q]
        if not np.any(aw):
            m_ref = int(self.z_kap[q] >> 1) & 1
            beta_Z_words = self.z2z[q].copy()
            reset_flip_ref = False
            if is_reset and m_ref == 1:
                self.z_kap[q] = (self.z_kap[q] + 2) & 3
                self._inv_dirty = True
                reset_flip_ref = True
            return DetMeasMeta(
                is_random=False,
                q=q,
                m_ref=m_ref,
                beta_Z_words=beta_Z_words,
                reset_flip_ref=reset_flip_ref,
            )

        nz_w = int(np.flatnonzero(aw)[0])
        low_val = int(aw[nz_w])
        p = (nz_w << 6) + (low_val & -low_val).bit_length() - 1
        p_w, p_b = p >> 6, p & 63
        p_mask = np.uint64(1 << p_b)

        G_x_indices = np.flatnonzero((self.z2x[:, p_w] & p_mask) != 0)
        G_z_indices = np.flatnonzero((self.x2x[:, p_w] & p_mask) != 0)

        a_words = aw.copy()
        b_rot_words = self.z2z[q].copy()
        b_rot_words[p_w] ^= p_mask
        e_p_words = np.zeros(self.n_words, dtype=np.uint64)
        e_p_words[p_w] = p_mask

        m_ref = 0 if is_reset else int(m_ref_override)
        kap = int(self.z_kap[q])
        kappa_rot = (kap + 1) & 3
        ab_rot_dot = int(np.bitwise_count(a_words & b_rot_words).sum()) & 1
        kap_iprot = ((m_ref << 1) - kap + (ab_rot_dot << 1)) & 3

        # Step A1.5: Direct rank-1 transvection Q_r -> (i P_rot) Q_r on anticommuting rows
        gx = (
            np.bitwise_count(
                (self.x2x & b_rot_words) ^ (self.x2z & a_words)
            ).sum(axis=1)
            & 1
        ) != 0
        ix = np.flatnonzero(gx)
        if len(ix):
            dpx = np.bitwise_count(self.x2x[ix] & b_rot_words).sum(axis=1) & 1
            self.x_kap[ix] = (kap_iprot + self.x_kap[ix] + (dpx << 1)) & 3
            self.x2x[ix] ^= a_words
            self.x2z[ix] ^= b_rot_words

        gz = (
            np.bitwise_count(
                (self.z2x & b_rot_words) ^ (self.z2z & a_words)
            ).sum(axis=1)
            & 1
        ) != 0
        iz = np.flatnonzero(gz)
        if len(iz):
            dpz = np.bitwise_count(self.z2x[iz] & b_rot_words).sum(axis=1) & 1
            self.z_kap[iz] = (kap_iprot + self.z_kap[iz] + (dpz << 1)) & 3
            self.z2x[iz] ^= a_words
            self.z2z[iz] ^= b_rot_words

        self._inv_dirty = True

        reset_flip_ref = False
        if is_reset and m_ref == 1:
            self.z_kap[q] = (self.z_kap[q] + 2) & 3
            reset_flip_ref = True

        return RandMeasMeta(
            is_random=True,
            q=q,
            m_ref=m_ref,
            p=p,
            a_words=a_words,
            b_rot_words=b_rot_words,
            e_p_words=e_p_words,
            kappa_rot=kappa_rot,
            G_x_indices=G_x_indices,
            G_z_indices=G_z_indices,
            reset_flip_ref=reset_flip_ref,
        )

    def copy(self) -> "ReferenceCHPTableau":
        clone = ReferenceCHPTableau(self.num_qubits)
        np.copyto(clone.x2x, self.x2x)
        np.copyto(clone.x2z, self.x2z)
        np.copyto(clone.z2x, self.z2x)
        np.copyto(clone.z2z, self.z2z)
        np.copyto(clone.x_kap, self.x_kap)
        np.copyto(clone.z_kap, self.z_kap)
        clone._inv_dirty = self._inv_dirty
        return clone

    def peek_pauli_expectation(
        self, pauli_terms: Sequence[tuple[int, str]]
    ) -> list[int]:
        """Read-only evaluation of Pauli expectation <psi| prod_{(q, P)} P_q |psi> in {+1, -1, 0} by accumulating row products into temporary scratch vectors without mutating internal tables."""
        if len(pauli_terms) == 0:
            return [1]
        if len(pauli_terms) == 1:
            q, p_char = pauli_terms[0]
            p_upper = p_char.upper()
            if p_upper == "I":
                return [1]
            if p_upper == "Z":
                if np.any(self.z2x[q]):
                    return [0]
                return [-1 if (int(self.z_kap[q]) & 3) == 2 else 1]
            if p_upper == "X":
                if np.any(self.x2x[q]):
                    return [0]
                return [-1 if (int(self.x_kap[q]) & 3) == 2 else 1]
            if p_upper == "Y":
                aw_y = self.x2x[q] ^ self.z2x[q]
                if np.any(aw_y):
                    return [0]
                dp = int(np.bitwise_count(self.x2z[q] & self.z2x[q]).sum()) & 1
                kap_y = (
                    1 + int(self.x_kap[q]) + int(self.z_kap[q]) + (dp << 1)
                ) & 3
                return [-1 if kap_y == 2 else 1]
            raise ValueError(f"Unrecognised Pauli character: {p_char}")

        aw_acc = np.zeros(self.n_words, dtype=np.uint64)
        bw_acc = np.zeros(self.n_words, dtype=np.uint64)
        kap_acc = 0
        for q, p_char in pauli_terms:
            p_upper = p_char.upper()
            if p_upper == "I":
                continue
            if p_upper == "Z":
                aw_t = self.z2x[q]
                bw_t = self.z2z[q]
                kap_t = int(self.z_kap[q])
            elif p_upper == "X":
                aw_t = self.x2x[q]
                bw_t = self.x2z[q]
                kap_t = int(self.x_kap[q])
            elif p_upper == "Y":
                aw_t = self.x2x[q] ^ self.z2x[q]
                bw_t = self.x2z[q] ^ self.z2z[q]
                dp = int(np.bitwise_count(self.x2z[q] & self.z2x[q]).sum()) & 1
                kap_t = (
                    1 + int(self.x_kap[q]) + int(self.z_kap[q]) + (dp << 1)
                ) & 3
            else:
                raise ValueError(f"Unrecognised Pauli character: {p_char}")

            dp_acc = int(np.bitwise_count(bw_acc & aw_t).sum()) & 1
            aw_acc ^= aw_t
            bw_acc ^= bw_t
            kap_acc = (kap_acc + kap_t + (dp_acc << 1)) & 3

        if np.any(aw_acc):
            return [0]
        return [-1 if (kap_acc & 3) == 2 else 1]

    def extract_clifford_cocycle(
        self,
        gate_name_or_ops: (
            str | stim.CircuitInstruction | Sequence[stim.CircuitInstruction]
        ),
        targets: Sequence[int] | None = None,
    ) -> InverseCliffordCocycle:
        """Read-only extraction of the compiled MRQF fault factors for E = U_gate^dagger on targets."""
        if isinstance(gate_name_or_ops, str):
            assert targets is not None
            gate_name = gate_name_or_ops
            target_group = tuple(int(q) for q in targets)
        elif isinstance(gate_name_or_ops, stim.CircuitInstruction):
            gate_name = gate_name_or_ops.name
            target_group = tuple(
                int(t.qubit_value) for t in gate_name_or_ops.targets_copy()
            )
        else:
            ops_list = list(gate_name_or_ops)
            assert len(ops_list) > 0
            gate_name = ops_list[0].name
            target_group = tuple(
                int(t.qubit_value) for t in ops_list[0].targets_copy()
            )
        step = UnitaryStepMeta(
            kind="unitary",
            op=stim.CircuitInstruction(gate_name, list(target_group)),
            gate_name=gate_name,
            num_qubits=self.num_qubits,
            target_groups=[target_group],
            _packed_words=(
                self.x2x,
                self.x2z,
                self.z2x,
                self.z2z,
                self.x_kap,
                self.z_kap,
            ),
            are_groups_disjoint=True,
        )
        return step.get_inverse_cocycle(0, self.num_qubits, self.n_words)


class CompiledReferenceCircuit:
    """Precompiles a stim.Circuit into a Phase 0 reference step tape without stim.Tableau conjugation."""

    def __init__(self, circuit: stim.Circuit) -> None:
        self.circuit = circuit
        self.num_qubits = circuit.num_qubits
        self.n_words = max(1, (self.num_qubits + 63) // 64)
        self.ref_chp = ReferenceCHPTableau(self.num_qubits)
        self._last_packed_snapshot = self.ref_chp.snapshot_packed()
        self._initial_packed_words = self._last_packed_snapshot
        self.steps: list[StepMeta] = []
        self._compile_block(circuit)

    def _get_packed_snapshot(self) -> PackedSymplecticTables:
        if self.ref_chp._inv_dirty:
            self._last_packed_snapshot = self.ref_chp.snapshot_packed()
            self.ref_chp._inv_dirty = False
        return self._last_packed_snapshot

    def _compile_block(
        self,
        block: stim.Circuit | stim.CircuitRepeatBlock | stim.CircuitInstruction,
    ) -> None:
        if isinstance(block, stim.Circuit):
            for op in block:
                self._compile_block(op)
        elif isinstance(block, stim.CircuitRepeatBlock):
            body = block.body_copy()
            for _ in range(block.repeat_count):
                self._compile_block(body)
        elif isinstance(block, stim.CircuitInstruction):
            if block.name in ("QUBIT_COORDS", "SHIFT_COORDS"):
                return
            self.compile_and_append_instruction(block)

    def compile_and_append_instruction(
        self, op: stim.CircuitInstruction
    ) -> StepMeta:
        """Compile a single non-coordinate instruction and advance the live ReferenceCHPTableau."""
        name = op.name
        gd = stim.gate_data(name)
        prev_step = self.steps[-1] if self.steps else None

        if gd.is_reset or gd.produces_measurements:
            if name == "MPAD":
                step = NoopStepMeta(
                    kind="noop",
                    op=op,
                    _packed_words=(
                        None
                        if self.ref_chp._inv_dirty
                        else self._last_packed_snapshot
                    ),
                    _prev_step=prev_step,
                    num_qubits=self.num_qubits,
                )
                self.steps.append(step)
                return step

            if prev_step is not None and prev_step._packed_words is None:
                prev_step._packed_words = self._get_packed_snapshot()

            substeps: list[MeasSubStepMeta] = []
            raw_targets = op.targets_copy()

            if name == "MPP":
                idx = 0
                product_terms: list[list[stim.GateTarget]] = []
                while idx < len(raw_targets):
                    term = [raw_targets[idx]]
                    idx += 1
                    while (
                        idx < len(raw_targets) and raw_targets[idx].is_combiner
                    ):
                        term.append(raw_targets[idx + 1])
                        idx += 2
                    product_terms.append(term)

                for term in product_terms:
                    invert_res = False
                    pre_ops: list[stim.CircuitInstruction] = []
                    post_ops: list[stim.CircuitInstruction] = []
                    term_qubits: list[int] = []
                    for gt in term:
                        q_r = gt.qubit_value
                        term_qubits.append(q_r)
                        if gt.is_inverted_result_target:
                            invert_res = not invert_res
                        if gt.is_x_target:
                            h_op = stim.CircuitInstruction("H", [q_r])
                            pre_ops.append(h_op)
                            post_ops.insert(0, h_op)
                        elif gt.is_y_target:
                            sdag_op = stim.CircuitInstruction("S_DAG", [q_r])
                            h_op = stim.CircuitInstruction("H", [q_r])
                            s_op = stim.CircuitInstruction("S", [q_r])
                            pre_ops.extend([sdag_op, h_op])
                            post_ops = [h_op, s_op] + post_ops
                    q0 = term_qubits[0]
                    cx_ops: list[stim.CircuitInstruction] = []
                    for q_r in term_qubits[1:]:
                        cx_ops.append(stim.CircuitInstruction("CX", [q_r, q0]))
                    pre_ops.extend(cx_ops)
                    post_ops = list(reversed(cx_ops)) + post_ops

                    for p_op in pre_ops:
                        self.ref_chp.do_unitary(
                            p_op.name,
                            [t.qubit_value for t in p_op.targets_copy()],
                        )
                    meta = self.ref_chp.measure_z(
                        q0, is_reset=False, m_ref_override=0
                    )
                    for p_op in post_ops:
                        self.ref_chp.do_unitary(
                            p_op.name,
                            [t.qubit_value for t in p_op.targets_copy()],
                        )
                    substeps.append(
                        MeasSubStepMeta(
                            q=q0,
                            invert_result=invert_res,
                            pre_ops=pre_ops,
                            post_ops=post_ops,
                            meta=meta,
                        )
                    )
            elif name in ("MXX", "MYY", "MZZ"):
                basis_pair = name[-1]
                for i in range(0, len(raw_targets), 2):
                    gt0, gt1 = raw_targets[i], raw_targets[i + 1]
                    q0, q1 = gt0.qubit_value, gt1.qubit_value
                    invert_res = bool(
                        gt0.is_inverted_result_target
                        ^ gt1.is_inverted_result_target
                    )
                    pre_ops = []
                    post_ops = []
                    if basis_pair == "X":
                        h_op = stim.CircuitInstruction("H", [q0, q1])
                        pre_ops.append(h_op)
                        post_ops.append(h_op)
                    elif basis_pair == "Y":
                        sdag_op = stim.CircuitInstruction("S_DAG", [q0, q1])
                        h_op = stim.CircuitInstruction("H", [q0, q1])
                        s_op = stim.CircuitInstruction("S", [q0, q1])
                        pre_ops.extend([sdag_op, h_op])
                        post_ops.extend([h_op, s_op])
                    cx_op = stim.CircuitInstruction("CX", [q1, q0])
                    pre_ops.append(cx_op)
                    post_ops.insert(0, cx_op)

                    for p_op in pre_ops:
                        self.ref_chp.do_unitary(
                            p_op.name,
                            [t.qubit_value for t in p_op.targets_copy()],
                        )
                    meta = self.ref_chp.measure_z(
                        q0, is_reset=False, m_ref_override=0
                    )
                    for p_op in post_ops:
                        self.ref_chp.do_unitary(
                            p_op.name,
                            [t.qubit_value for t in p_op.targets_copy()],
                        )
                    substeps.append(
                        MeasSubStepMeta(
                            q=q0,
                            invert_result=invert_res,
                            pre_ops=pre_ops,
                            post_ops=post_ops,
                            meta=meta,
                        )
                    )
            else:
                basis: Literal["X", "Y", "Z"] = "Z"
                if name.endswith("X"):
                    basis = "X"
                elif name.endswith("Y"):
                    basis = "Y"

                if basis == "Z":
                    for gt in raw_targets:
                        q = gt.qubit_value
                        invert_res = bool(gt.is_inverted_result_target)
                        meta = self.ref_chp.measure_z(
                            q, is_reset=gd.is_reset, m_ref_override=0
                        )
                        substeps.append(
                            MeasSubStepMeta(
                                q=q,
                                invert_result=invert_res,
                                pre_ops=[],
                                post_ops=[],
                                meta=meta,
                            )
                        )
                else:
                    for gt in raw_targets:
                        q = gt.qubit_value
                        invert_res = bool(gt.is_inverted_result_target)
                        pre_ops = []
                        post_ops = []
                        if basis == "X":
                            h_op = stim.CircuitInstruction("H", [q])
                            pre_ops.append(h_op)
                            post_ops.append(h_op)
                        elif basis == "Y":
                            sdag_op = stim.CircuitInstruction("S_DAG", [q])
                            h_op = stim.CircuitInstruction("H", [q])
                            s_op = stim.CircuitInstruction("S", [q])
                            pre_ops.extend([sdag_op, h_op])
                            post_ops.extend([h_op, s_op])

                        for p_op in pre_ops:
                            self.ref_chp.do_unitary(
                                p_op.name,
                                [t.qubit_value for t in p_op.targets_copy()],
                            )
                        meta = self.ref_chp.measure_z(
                            q, is_reset=gd.is_reset, m_ref_override=0
                        )
                        for p_op in post_ops:
                            self.ref_chp.do_unitary(
                                p_op.name,
                                [t.qubit_value for t in p_op.targets_copy()],
                            )
                        substeps.append(
                            MeasSubStepMeta(
                                q=q,
                                invert_result=invert_res,
                                pre_ops=pre_ops,
                                post_ops=post_ops,
                                meta=meta,
                            )
                        )

            step = MeasStepMeta(
                kind="meas",
                op=op,
                gate_name=name,
                produces_measurements=gd.produces_measurements,
                is_reset=gd.is_reset,
                substeps=substeps,
                _packed_words=self._get_packed_snapshot(),
                num_qubits=self.num_qubits,
            )
            self.steps.append(step)
            return step

        if gd.is_unitary:
            raw_t = op.targets_copy()
            if gd.is_two_qubit_gate and any(
                t.qubit_value is None for t in raw_t
            ):
                quantum_targets: list[int] = []
                for i in range(0, len(raw_t), 2):
                    q0, q1 = raw_t[i].qubit_value, raw_t[i + 1].qubit_value
                    if q0 is not None and q1 is not None:
                        quantum_targets.extend([q0, q1])
                if not quantum_targets:
                    step = NoopStepMeta(
                        kind="noop",
                        op=op,
                        _packed_words=(
                            None
                            if self.ref_chp._inv_dirty
                            else self._last_packed_snapshot
                        ),
                        _prev_step=prev_step,
                        num_qubits=self.num_qubits,
                    )
                    self.steps.append(step)
                    return step
                targets = quantum_targets
                op = stim.CircuitInstruction(
                    name, quantum_targets, op.gate_args_copy()
                )
            else:
                targets = [t.qubit_value for t in raw_t]
            arity = 2 if gd.is_two_qubit_gate else 1
            groups = [
                tuple(targets[i : i + arity])
                for i in range(0, len(targets), arity)
            ]
            is_disjoint = len(set(targets)) == len(targets)
            if is_disjoint:
                self.ref_chp.do_unitary(name, targets)
                packed_after = self._get_packed_snapshot()
                group_packed_words: list[PackedSymplecticTables] | None = None
            else:
                group_packed_words = []
                for grp in groups:
                    self.ref_chp.do_unitary(name, grp)
                    group_packed_words.append(self.ref_chp.snapshot_packed())
                self._last_packed_snapshot = group_packed_words[-1]
                self.ref_chp._inv_dirty = False
                packed_after = self._last_packed_snapshot
            step = UnitaryStepMeta(
                kind="unitary",
                op=op,
                gate_name=name,
                num_qubits=self.num_qubits,
                target_groups=groups,
                _group_packed_words=group_packed_words,
                _packed_words=packed_after,
                _prev_step=prev_step,
                are_groups_disjoint=is_disjoint,
            )
            self.steps.append(step)
            return step

        step = NoopStepMeta(
            kind="noop",
            op=op,
            _packed_words=(
                None if self.ref_chp._inv_dirty else self._last_packed_snapshot
            ),
            _prev_step=prev_step,
            num_qubits=self.num_qubits,
        )
        self.steps.append(step)
        return step
