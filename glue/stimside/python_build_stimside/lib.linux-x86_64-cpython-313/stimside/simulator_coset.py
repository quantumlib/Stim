from __future__ import annotations

import ctypes
import dataclasses
import itertools as it
from typing import Callable, Iterable, Literal, Sequence

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.coset_kernels import (
    get_coset_kernels_lib,
    i8_p,
    i32_p,
    i64_p,
    int_p,
    u8_p,
    u64_p,
)
from stimside.util.reference_chp import (
    CompiledReferenceCircuit,
    DetMeasMeta,
    InverseCliffordCocycle,
    MeasStepMeta,
    PackedSymplecticTables,
    RandMeasMeta,
    StepMeta,
    UnitaryStepMeta,
    _bools_to_uint64_words,
)


def _ensure_tableside_deterministic_seeding() -> None:
    """Ensure TablesideSimulator seeds its internal TableauSimulator when seed is provided, without modifying simulator_tableau.py."""
    if getattr(TablesideSimulator, "_coset_seed_patched", False):
        return

    orig_init = TablesideSimulator.__init__
    orig_clear = TablesideSimulator.clear
    orig_run = TablesideSimulator.run

    def _patched_init(self: TablesideSimulator, *args: object, **kwargs: object) -> None:
        orig_init(self, *args, **kwargs)  # type: ignore[arg-type]
        self._runs_completed = 0  # type: ignore[attr-defined]
        self._init_running_tableau = self._running_tableau  # type: ignore[attr-defined]
        if self.seed is not None:
            shot_seed = int(self.seed)
            self.np_rng = np.random.default_rng(seed=shot_seed)
            self._tableau_simulator = stim.TableauSimulator(seed=shot_seed)

    def _patched_clear(self: TablesideSimulator) -> None:
        orig_clear(self)
        self._detector_flips = None
        self._observable_flips = None
        self._running_tableau = getattr(self, "_init_running_tableau", True)
        self._finished_running_circuit = False
        if self.seed is not None:
            runs_done = getattr(self, "_runs_completed", 0)
            shot_seed = int(self.seed) + int(runs_done)
            self.np_rng = np.random.default_rng(seed=shot_seed)
            self._tableau_simulator = stim.TableauSimulator(seed=shot_seed)

    def _patched_run(self: TablesideSimulator) -> None:
        orig_run(self)
        self._runs_completed = getattr(self, "_runs_completed", 0) + 1  # type: ignore[attr-defined]

    TablesideSimulator.__init__ = _patched_init  # type: ignore[method-assign]
    TablesideSimulator.clear = _patched_clear  # type: ignore[method-assign]
    TablesideSimulator.run = _patched_run  # type: ignore[method-assign]
    TablesideSimulator._coset_seed_patched = True  # type: ignore[attr-defined]


_ensure_tableside_deterministic_seeding()


def _word_dot_parity(
    u_words: NDArray[np.uint64], v_words: NDArray[np.uint64]
) -> int:
    """Compute (u . v) mod 2 across uint64 words without allocating temporary NumPy arrays."""
    acc = 0
    for a, b in zip(u_words.tolist(), v_words.tolist()):
        acc ^= a & b
    return acc.bit_count() & 1


def _words_any_overlap(
    u_words: NDArray[np.uint64], v_words: NDArray[np.uint64]
) -> bool:
    """Return True iff u_words and v_words share at least one set bit."""
    for a, b in zip(u_words.tolist(), v_words.tolist()):
        if a & b:
            return True
    return False


def _words_any_nonzero(u_words: NDArray[np.uint64]) -> bool:
    """Return True iff u_words has at least one non-zero bit."""
    for a in u_words.tolist():
        if a:
            return True
    return False


def _first_set_bit_in_words(col_words: NDArray[np.uint64]) -> int:
    """Return the index of the lowest set bit across col_words, or -1 if all zero."""
    for w, val in enumerate(col_words.tolist()):
        if val:
            return (val & -val).bit_length() - 1 + (w << 6)
    return -1


class MRQFState:
    """Compressed Moving-Reference Quadratic-Form (MRQF) state D = (h, A, ell, Gamma, P).

    Represents the residual stabilizer state in reference coordinates B_t:
        |chi> = 2^{-d/2} sum_{u in F_2^d} i^{Q(u)} |h xor A u>,
        Q(u) = sum_{j=0}^{d-1} ell_j u_j + 2 sum_{0 <= j < k < d} Gamma_{jk} u_j u_k (mod 4),
    maintaining the Principal-Row Invariant A_{P, :} = I_d in O(n(d + 1) + d^2) bits.
    """

    def __init__(
        self,
        num_qubits: int,
        n_words: int | None = None,
        _init_cap: int = 16,
    ) -> None:
        self.num_qubits = num_qubits
        self.n_words = (
            max(1, (num_qubits + 63) // 64) if n_words is None else n_words
        )
        self._cap = max(4, _init_cap)
        self._d = 0
        self.h_words: NDArray[np.uint64] = np.zeros(
            self.n_words, dtype=np.uint64
        )
        self.a_union: NDArray[np.uint64] = np.zeros(
            self.n_words, dtype=np.uint64
        )
        self._A_buf: NDArray[np.uint64] = np.zeros(
            (self._cap, self.n_words), dtype=np.uint64
        )
        self._ell_buf: NDArray[np.uint8] = np.zeros(
            (self._cap,), dtype=np.uint8
        )
        self._Gamma_buf: NDArray[np.uint8] = np.zeros(
            (self._cap, self._cap), dtype=np.uint8
        )
        self._A_view: NDArray[np.uint64] = self._A_buf[:0]
        self._ell_view: NDArray[np.uint8] = self._ell_buf[:0]
        self._Gamma_view: NDArray[np.uint8] = self._Gamma_buf[:0, :0]
        self.P: list[int] = []

    def _refresh_views(self) -> None:
        d = self._d
        self._A_view = self._A_buf[:d]
        self._ell_view = self._ell_buf[:d]
        self._Gamma_view = self._Gamma_buf[:d, :d]

    @property
    def A_words(self) -> NDArray[np.uint64]:
        return self._A_view

    @A_words.setter
    def A_words(self, val: NDArray[np.uint64]) -> None:
        arr = np.asarray(val, dtype=np.uint64)
        d_new = arr.shape[0]
        self._ensure_capacity(d_new)
        self._d = d_new
        if d_new > 0:
            self._A_buf[:d_new] = arr
        self._refresh_views()
        self._recompute_a_union()

    @property
    def ell(self) -> NDArray[np.uint8]:
        return self._ell_view

    @ell.setter
    def ell(self, val: NDArray[np.uint8]) -> None:
        arr = np.asarray(val, dtype=np.uint8)
        d_new = arr.shape[0]
        self._ensure_capacity(d_new)
        self._d = d_new
        if d_new > 0:
            self._ell_buf[:d_new] = arr
        self._refresh_views()

    @property
    def Gamma(self) -> NDArray[np.uint8]:
        return self._Gamma_view

    @Gamma.setter
    def Gamma(self, val: NDArray[np.uint8]) -> None:
        arr = np.asarray(val, dtype=np.uint8)
        d_new = arr.shape[0]
        self._ensure_capacity(d_new)
        self._d = d_new
        if d_new > 0:
            self._Gamma_buf[:d_new, :d_new] = arr
        self._refresh_views()

    def _ensure_capacity(self, need_d: int) -> None:
        if need_d <= self._cap:
            return
        new_cap = self._cap
        while new_cap < need_d:
            new_cap *= 2
        new_A = np.zeros((new_cap, self.n_words), dtype=np.uint64)
        new_ell = np.zeros((new_cap,), dtype=np.uint8)
        new_Gamma = np.zeros((new_cap, new_cap), dtype=np.uint8)
        d = self._d
        if d > 0:
            new_A[:d] = self._A_buf[:d]
            new_ell[:d] = self._ell_buf[:d]
            new_Gamma[:d, :d] = self._Gamma_buf[:d, :d]
        self._cap = new_cap
        self._A_buf = new_A
        self._ell_buf = new_ell
        self._Gamma_buf = new_Gamma
        self._refresh_views()

    def _recompute_a_union(self) -> None:
        d = self._d
        if d == 0:
            self.a_union.fill(0)
        elif d == 1:
            np.copyto(self.a_union, self._A_buf[0])
        else:
            np.bitwise_or.reduce(self._A_buf[:d], axis=0, out=self.a_union)

    @property
    def d(self) -> int:
        return self._d

    @property
    def num_frames(self) -> int:
        return 1 << self._d

    def copy(self) -> "MRQFState":
        clone = MRQFState(self.num_qubits, self.n_words, _init_cap=self._cap)
        clone._d = self._d
        np.copyto(clone.h_words, self.h_words)
        np.copyto(clone.a_union, self.a_union)
        if self._d > 0:
            clone._A_buf[: self._d] = self._A_buf[: self._d]
            clone._ell_buf[: self._d] = self._ell_buf[: self._d]
            clone._Gamma_buf[: self._d, : self._d] = self._Gamma_buf[
                : self._d, : self._d
            ]
        clone.P = list(self.P)
        clone._refresh_views()
        return clone

    def _sync_external_if_needed(self) -> None:
        if (
            self._d != len(self.P)
            or self.A_words.base is not self._A_buf
            or self.ell.base is not self._ell_buf
        ):
            k = len(self.P)
            self._ensure_capacity(k)
            if k > 0:
                self._A_buf[:k] = self.A_words
                self._ell_buf[:k] = self.ell
                self._Gamma_buf[:k, :k] = self.Gamma
            self._d = k
            self._refresh_views()
        self._recompute_a_union()

    def check_invariants(self) -> None:
        """Verify Check C.1: shapes, principal-row identity A_{P, :} = I_d, symmetric zero-diagonal Gamma, ell in Z_4."""
        self._sync_external_if_needed()
        d = len(self.P)
        assert self._d == d
        assert self.h_words.shape == (self.n_words,)
        assert self.A_words.shape == (d, self.n_words)
        assert self.ell.shape == (d,)
        assert self.Gamma.shape == (d, d)
        assert len(set(self.P)) == d
        if d == 0:
            assert not np.any(self.a_union)
        else:
            expected_union = np.bitwise_or.reduce(self.A_words, axis=0)
            assert np.array_equal(self.a_union, expected_union)
            assert np.all(self.ell < 4)
            assert np.all((self.Gamma == 0) | (self.Gamma == 1))
            assert np.array_equal(self.Gamma, self.Gamma.T)
            assert np.all(np.diag(self.Gamma) == 0)
            for j, p_j in enumerate(self.P):
                p_word = p_j >> 6
                p_mask = np.uint64(1 << (p_j & 63))
                col_bits = (self.A_words[:, p_word] & p_mask) != 0
                expected = np.zeros(d, dtype=np.bool_)
                expected[j] = True
                assert np.array_equal(col_bits, expected)

    def _matvec_A_T(self, b_words: NDArray[np.uint64]) -> NDArray[np.uint8]:
        """Compute t = A^T b in F_2^d."""
        d = self._d
        if d <= 8:
            b_list = b_words.tolist()
            au_list = self.a_union.tolist()
            nz_w = [
                (w, b_list[w])
                for w in range(self.n_words)
                if b_list[w] & au_list[w]
            ]
            out = np.zeros(d, dtype=np.uint8)
            if not nz_w:
                return out
            A_list = self._A_buf[:d].tolist()
            for r in range(d):
                row = A_list[r]
                acc = 0
                for w, bw in nz_w:
                    acc ^= row[w] & bw
                out[r] = acc.bit_count() & 1
            return out
        if self.n_words == 1:
            return (
                np.bitwise_count(self.A_words[:, 0] & b_words[0]) & 1
            ).astype(np.uint8)
        return (
            np.bitwise_count(self.A_words & b_words[None, :]).sum(axis=1) & 1
        ).astype(np.uint8)

    # S1. AddColumn(r, s)
    def s1_add_column(self, r: int, s: int) -> None:
        """Add column r into column s and update ell and Gamma consistently (Eq. 15)."""
        assert r != s
        d = self._d
        ell_r = int(self._ell_buf[r])
        ell_s = int(self._ell_buf[s])
        gamma_rs = int(self._Gamma_buf[r, s])

        self._A_buf[s] ^= self._A_buf[r]
        self._ell_buf[s] = (ell_s + ell_r + (gamma_rs << 1)) & 3
        new_gamma_rs = gamma_rs ^ (ell_r & 1)
        self._Gamma_buf[s, :d] ^= self._Gamma_buf[r, :d]
        self._Gamma_buf[:d, s] = self._Gamma_buf[s, :d]
        self._Gamma_buf[r, s] = new_gamma_rs
        self._Gamma_buf[s, r] = new_gamma_rs
        self._Gamma_buf[s, s] = 0

    # S2. MakePrincipal(j, p)
    def s2_make_principal(self, j: int, p: int) -> None:
        """Restore designated identity row p for column j (Section 6, S2)."""
        p_word = p >> 6
        p_bit = p & 63
        A_buf = self._A_buf
        for s in range(self._d):
            if s != j and ((int(A_buf[s, p_word]) >> p_bit) & 1):
                self.s1_add_column(j, s)
        self.P[j] = p

    # S4. DropVariable(j)
    def s4_drop_variable(self, j: int) -> None:
        """Remove variable slot j from A_words, ell, Gamma, and P in-place (Eq. 17)."""
        self._sync_external_if_needed()
        k = self._d
        if k == 1:
            self._d = 0
            self.P.clear()
            self.a_union.fill(0)
            self._refresh_views()
            return
        if j < k - 1:
            self._A_buf[j : k - 1] = self._A_buf[j + 1 : k]
            self._ell_buf[j : k - 1] = self._ell_buf[j + 1 : k]
            self._Gamma_buf[j : k - 1, :k] = self._Gamma_buf[j + 1 : k, :k]
            self._Gamma_buf[: k - 1, j : k - 1] = self._Gamma_buf[
                : k - 1, j + 1 : k
            ]
        self._d = k - 1
        self.P.pop(j)
        self._refresh_views()
        self._recompute_a_union()

    # S3. Restrict(lambda, epsilon)
    def s3_restrict(
        self, lam: NDArray[np.uint8] | Sequence[int], eps: int
    ) -> tuple[bool, float]:
        """Impose affine equation lam^T u = eps (mod 2) on the support (Eq. 16)."""
        self._sync_external_if_needed()
        d = self._d
        first_nz = -1
        for idx in range(d):
            if lam[idx]:
                if first_nz == -1:
                    first_nz = idx
                else:
                    self.s1_add_column(first_nz, idx)
        eps_bit = int(eps) & 1
        if first_nz == -1:
            return (True, 1.0) if eps_bit == 0 else (False, 0.0)
        if eps_bit:
            self.h_words ^= self._A_buf[first_nz]
            self._ell_buf[:d] = (
                self._ell_buf[:d] + (self._Gamma_buf[first_nz, :d] << 1)
            ) & 3
        self.s4_drop_variable(first_nz)
        return True, 0.5

    # S5. EliminateZero(j)
    def s5_eliminate_zero(self, j: int) -> tuple[bool, int]:
        """Sum out redundant variable j with A_j = 0 coherently (Eq. 19-20)."""
        self._sync_external_if_needed()
        c = int(self._ell_buf[j])
        k = self._d
        lam = np.empty(k - 1, dtype=np.uint8)
        if j > 0:
            lam[:j] = self._Gamma_buf[:j, j]
        if j < k - 1:
            lam[j:] = self._Gamma_buf[j + 1 : k, j]
        self.s4_drop_variable(j)

        if c & 1:
            if len(lam) > 0:
                self.ell[:] = (
                    (self.ell.astype(np.int16) - c * lam.astype(np.int16)) & 3
                ).astype(np.uint8)
                outer = lam[:, None] & lam[None, :]
                np.fill_diagonal(outer, 0)
                self.Gamma ^= outer
            return True, 1

        if np.any(lam):
            ok, _ = self.s3_restrict(lam, c >> 1)
            return ok, (1 if ok else 0)

        if c == 0:
            return True, 2
        return False, 0

    # P1. ApplyPauli(a, b, kappa)
    def p1_apply_pauli(
        self,
        a_words: NDArray[np.uint64],
        b_words: NDArray[np.uint64],
        kappa: int = 0,
    ) -> None:
        """Apply P(a, b, kappa) to |chi> in O(d * n_words) time (Eq. 7)."""
        if self._d > 0 and _words_any_overlap(b_words, self.a_union):
            t = self._matvec_A_T(b_words)
            self.ell[:] = (self.ell + (t << 1)) & 3
        self.h_words ^= a_words

    def _normalize_column(self, idx: int) -> None:
        """Normalize newly added column idx against principal rows 0..idx-1 via S1, then S2 or S5."""
        A_buf = self._A_buf
        for r in range(idx):
            p_r = self.P[r]
            if (int(A_buf[idx, p_r >> 6]) >> (p_r & 63)) & 1:
                self.s1_add_column(r, idx)
        p_new = _first_set_bit_in_words(A_buf[idx])
        if p_new >= 0:
            self.s2_make_principal(idx, p_new)
        else:
            ok, eta = self.s5_eliminate_zero(idx)
            assert ok and eta == 1, (
                f"Unitary EliminateZero failed: ok={ok}, eta={eta}"
            )

    # P2. ApplyQuarterTurn(a, b, kappa)
    def p2_apply_quarter_turn(
        self,
        a_words: NDArray[np.uint64],
        b_words: NDArray[np.uint64],
        kappa: int,
    ) -> None:
        """Apply quarter-turn R(P(a, b, kappa)) = (I + i P(a, b, kappa))/sqrt(2) to |chi> (Eq. 8-9)."""
        d0 = self._d
        self._ensure_capacity(d0 + 1)
        if d0 > 0:
            if _words_any_overlap(b_words, self.a_union):
                t = self._matvec_A_T(b_words)
                self._Gamma_buf[:d0, d0] = t
                self._Gamma_buf[d0, :d0] = t
            else:
                self._Gamma_buf[:d0, d0] = 0
                self._Gamma_buf[d0, :d0] = 0
        self._Gamma_buf[d0, d0] = 0
        bh = _word_dot_parity(b_words, self.h_words)
        c = (kappa + 1 + (bh << 1)) & 3

        self._A_buf[d0] = a_words
        self.a_union |= a_words
        self._ell_buf[d0] = c
        self.P.append(-1)
        self._d = d0 + 1
        self._refresh_views()
        self._normalize_column(d0)

    # PCP. ApplyControlledPauli(a1, b1, kappa1, a2, b2, kappa2)
    def p2_apply_controlled_pauli(
        self,
        a1_words: NDArray[np.uint64],
        b1_words: NDArray[np.uint64],
        kappa1: int,
        a2_words: NDArray[np.uint64],
        b2_words: NDArray[np.uint64],
        kappa2: int,
    ) -> None:
        """Apply controlled-Pauli U = (I + P_1 + P_2 - P_1 P_2) / 2 via direct 2-column rank-2 injection."""
        d0 = self._d
        self._ensure_capacity(d0 + 2)
        if d0 > 0:
            if _words_any_overlap(b1_words, self.a_union):
                t1 = self._matvec_A_T(b1_words)
                self._Gamma_buf[:d0, d0] = t1
                self._Gamma_buf[d0, :d0] = t1
            else:
                self._Gamma_buf[:d0, d0] = 0
                self._Gamma_buf[d0, :d0] = 0
            if _words_any_overlap(b2_words, self.a_union):
                t2 = self._matvec_A_T(b2_words)
                self._Gamma_buf[:d0, d0 + 1] = t2
                self._Gamma_buf[d0 + 1, :d0] = t2
            else:
                self._Gamma_buf[:d0, d0 + 1] = 0
                self._Gamma_buf[d0 + 1, :d0] = 0

        bh1 = _word_dot_parity(b1_words, self.h_words)
        bh2 = _word_dot_parity(b2_words, self.h_words)
        c1 = (kappa1 + (bh1 << 1)) & 3
        c2 = (kappa2 + (bh2 << 1)) & 3
        gamma12 = 1 ^ _word_dot_parity(b1_words, a2_words)

        self._A_buf[d0] = a1_words
        self._A_buf[d0 + 1] = a2_words
        self.a_union |= a1_words
        self.a_union |= a2_words
        self._ell_buf[d0] = c1
        self._ell_buf[d0 + 1] = c2
        self._Gamma_buf[d0, d0] = 0
        self._Gamma_buf[d0 + 1, d0 + 1] = 0
        self._Gamma_buf[d0, d0 + 1] = gamma12
        self._Gamma_buf[d0 + 1, d0] = gamma12
        self.P.extend([-1, -1])
        self._d = d0 + 2
        self._refresh_views()

        self._normalize_column(d0)
        if self._d > 0 and self.P[-1] == -1:
            self._normalize_column(self._d - 1)

    # P3. MeasureDiagonal(b, sigma; optional requested bit)
    def p3_measure_diagonal(
        self,
        b_words: NDArray[np.uint64],
        sigma: int,
        draw_bit_fn: Callable[[], int] | None = None,
        requested_bit: int | None = None,
    ) -> tuple[int, float]:
        """Measure diagonal observable (-1)^sigma Z(b) on |chi> (Eq. 11)."""
        b_list = b_words.tolist()
        h_list = self.h_words.tolist()
        bh_acc = 0
        for bw, hw in zip(b_list, h_list):
            bh_acc ^= bw & hw
        bh = bh_acc.bit_count() & 1
        beta = (sigma ^ bh) & 1
        d = self._d
        if d == 0:
            if requested_bit is not None and (requested_bit & 1) != beta:
                return int(requested_bit) & 1, 0.0
            return beta, 1.0

        au_list = self.a_union.tolist()
        nz_w = [
            (w, b_list[w])
            for w in range(self.n_words)
            if b_list[w] & au_list[w]
        ]
        if not nz_w:
            if requested_bit is not None and (requested_bit & 1) != beta:
                return int(requested_bit) & 1, 0.0
            return beta, 1.0

        A_list = self._A_buf[:d].tolist()
        first_nz = -1
        other_nz: list[int] = []
        for r in range(d):
            row = A_list[r]
            acc = 0
            for w, bw in nz_w:
                acc ^= row[w] & bw
            if acc.bit_count() & 1:
                if first_nz == -1:
                    first_nz = r
                else:
                    other_nz.append(r)

        if first_nz == -1:
            if requested_bit is not None and (requested_bit & 1) != beta:
                return int(requested_bit) & 1, 0.0
            return beta, 1.0

        if requested_bit is not None:
            m = int(requested_bit) & 1
        elif draw_bit_fn is not None:
            m = int(draw_bit_fn()) & 1
        else:
            raise ValueError(
                "Either requested_bit or draw_bit_fn must be provided when lam != 0."
            )
        eps_bit = (m ^ beta) & 1
        for idx in other_nz:
            self.s1_add_column(first_nz, idx)
        if eps_bit:
            self.h_words ^= self._A_buf[first_nz]
            self._ell_buf[:d] = (
                self._ell_buf[:d] + (self._Gamma_buf[first_nz, :d] << 1)
            ) & 3
        self.s4_drop_variable(first_nz)
        return m, 0.5

    # Q1. PauliExpectation(a, b, kappa)
    def q1_pauli_expectation(
        self,
        a_words: NDArray[np.uint64],
        b_words: NDArray[np.uint64],
        kappa: int,
    ) -> int:
        """Evaluate exact read-only Pauli expectation <chi|P(a, b, kappa)|chi> in {0, +1, -1} (Eq. 14)."""
        d = self._d
        if d == 0:
            if np.any(a_words):
                return 0
            bh = _word_dot_parity(b_words, self.h_words)
            exp_mod4 = (kappa + (bh << 1)) & 3
            return 1 if exp_mod4 == 0 else -1

        if np.any(a_words & ~self.a_union):
            return 0

        s = np.array(
            [
                int((a_words[p_r >> 6] >> np.uint64(p_r & 63)) & np.uint64(1))
                for p_r in self.P
            ],
            dtype=np.uint8,
        )
        s_nz = np.flatnonzero(s)
        if len(s_nz) == 0:
            if np.any(a_words):
                return 0
        else:
            a_recon = np.bitwise_xor.reduce(self.A_words[s_nz], axis=0)
            if not np.array_equal(a_recon, a_words):
                return 0

        at_b = self._matvec_A_T(b_words)
        ws = ((self.ell & 1) & s) ^ ((self.Gamma @ s) & 1)
        if not np.array_equal(at_b, ws):
            return 0

        s_i32 = s.astype(np.int32)
        lin = int(np.dot(self.ell.astype(np.int32), s_i32))
        quad = int(s_i32 @ np.triu(self.Gamma, k=1).astype(np.int32) @ s_i32)
        q_s = (lin + (quad << 1)) & 3
        bh = _word_dot_parity(b_words, self.h_words)
        exp_mod4 = (kappa + (bh << 1) - q_s) & 3
        assert exp_mod4 in (0, 2)
        return 1 if exp_mod4 == 0 else -1


ActiveCosetState = MRQFState


class CosetsideSimulator:
    """High-performance batched Clifford-error & stabilizer simulator via Moving-Reference Quadratic Forms (MRQF).

    Combines:
    1. Phase 0 Reference CHP Tableau (backed by C++ stim.Tableau),
    2. Batched C++ SIMD baseline Pauli frame tracking via stim.FlipSimulator across all B shots,
    3. Compressed polynomial-space MRQF states D_b = (h, A, ell, Gamma, P) only for active shots with d_b >= 1,
       with O(0) cost on ideal unitary gates and automatic O(1) self-healing eviction back into stim.FlipSimulator
       whenever d_b drops to 0.
    """

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        compiled_op_handler: CompiledOpHandler["CosetsideSimulator"],
        seed: int | None = None,
        batch_size: int = 1,
        compiled_ref: CompiledReferenceCircuit | None = None,
        sync_tableside_rng: bool | None = None,
        use_cpp_kernels: bool = True,
    ) -> None:
        self.circuit = circuit
        self.num_qubits = circuit.num_qubits
        self.seed = seed
        self.batch_size = batch_size
        self.sync_tableside_rng = (
            False if sync_tableside_rng is None else bool(sync_tableside_rng)
        )
        self.use_cpp_kernels = bool(use_cpp_kernels)
        self._n_words = max(1, (self.num_qubits + 63) // 64)
        self._batches_completed = 0
        self._tab_scratch_q = self.num_qubits

        self._shot_np_rngs: list[np.random.Generator] | None = None
        self._tab_rngs: list[stim.TableauSimulator] | None = None
        self._tab_rngs_x: list[stim.TableauSimulator] | None = None
        self._shot_tiebreak_rng: NDArray[np.uint64] = np.zeros(
            self.batch_size, dtype=np.uint64
        )
        self._cpp_lib: ctypes.CDLL | None = None
        self._cpp_eng: int | None = None
        self._cpp_dirty: bool = False
        self._py_dirty: bool = False
        self._any_x_c = ctypes.c_int(0)
        self._any_z_c = ctypes.c_int(0)
        self._ext_rand_bits: NDArray[np.uint8] | None = None
        self._ext_cursor_arr: NDArray[np.int32] = np.zeros(
            self.batch_size, dtype=np.int32
        )
        self._init_rngs()

        self._use_cpp = self.use_cpp_kernels and (self._tab_rngs is None)
        if self._use_cpp:
            self._cpp_lib = get_coset_kernels_lib()
            self._cpp_eng = self._cpp_lib.engine_create(
                self.num_qubits,
                self.batch_size,
                int(self.seed or 9015),
                self._shot_tiebreak_rng.ctypes.data_as(u64_p),
            )

        self._flip_simulator = stim.FlipSimulator(
            batch_size=self.batch_size,
            disable_stabilizer_randomization=True,
            num_qubits=self.num_qubits,
            seed=self.seed,
        )

        self._compiled_ref = (
            compiled_ref
            if compiled_ref is not None
            else CompiledReferenceCircuit(circuit)
        )
        self.chp = self._compiled_ref.ref_chp
        self._compiled_m2d_converter = circuit.compile_m2d_converter()
        self._compiled_op_handler = compiled_op_handler
        self.compiled_op_handler = compiled_op_handler

        self.qubit_coords: dict[int, list[float]] = {}
        self.qubit_tags: dict[int, str] = {}
        self.coords_shifts: list[float] = []

        self._active_shots: dict[int, MRQFState] = {}
        self._measurement_columns: list[NDArray[np.bool_]] = []
        self._scratch_mask = np.zeros(
            (self.num_qubits, self.batch_size), dtype=np.bool_
        )
        self._scratch_x_mask = np.zeros(
            (self.num_qubits, self.batch_size), dtype=np.bool_
        )
        self._scratch_z_mask = np.zeros(
            (self.num_qubits, self.batch_size), dtype=np.bool_
        )

        self._circuit_time = 0
        self._step_cursor = 0
        self._used_interactively = False
        self._finished_running_circuit = False

        self._final_measurement_records: NDArray[np.bool_] | None = None
        self._detector_flips: NDArray[np.bool_] | None = None
        self._observable_flips: NDArray[np.bool_] | None = None

    def __del__(self) -> None:
        if getattr(self, "_cpp_eng", None) is not None and getattr(self, "_cpp_lib", None) is not None:
            try:
                self._cpp_lib.engine_destroy(self._cpp_eng)
            except Exception:
                pass
            self._cpp_eng = None

    @property
    def active_shots(self) -> dict[int, MRQFState]:
        if self._use_cpp:
            if self._cpp_dirty:
                self._sync_cpp_to_py()
            self._py_dirty = True
        return self._active_shots

    @active_shots.setter
    def active_shots(self, val: dict[int, MRQFState]) -> None:
        self._active_shots = val
        if self._use_cpp:
            self._cpp_dirty = False
            self._py_dirty = True

    def _sync_cpp_to_py(self) -> None:
        if not self._cpp_dirty or self._cpp_eng is None or self._cpp_lib is None:
            return
        lib = self._cpp_lib
        out_idx = np.empty(self.batch_size, dtype=np.int32)
        out_d = np.empty(self.batch_size, dtype=np.int32)
        n_act = int(
            lib.engine_get_active_shot_indices(
                self._cpp_eng,
                out_idx.ctypes.data_as(i32_p),
                out_d.ctypes.data_as(i32_p),
            )
        )
        new_active_set = set(int(out_idx[i]) for i in range(n_act))
        for old_b in list(self._active_shots.keys()):
            if old_b not in new_active_set:
                del self._active_shots[old_b]
        for i in range(n_act):
            b = int(out_idx[i])
            d = int(out_d[i])
            st = self._active_shots.get(b)
            if st is None:
                st = MRQFState(
                    self.num_qubits, self._n_words, _init_cap=max(16, d)
                )
                self._active_shots[b] = st
            else:
                st._ensure_capacity(max(1, d))
            st._d = d
            out_P = np.empty(max(1, d), dtype=np.int32)
            out_Gamma = np.empty((max(1, d), max(1, d)), dtype=np.uint8)
            lib.engine_export_shot(
                self._cpp_eng,
                b,
                st.h_words.ctypes.data_as(u64_p),
                st.a_union.ctypes.data_as(u64_p),
                st._A_buf.ctypes.data_as(u64_p),
                st._ell_buf.ctypes.data_as(u8_p),
                out_Gamma.ctypes.data_as(u8_p),
                out_P.ctypes.data_as(i32_p),
            )
            if d > 0:
                st._Gamma_buf[:d, :d] = out_Gamma[:d, :d]
                st.P = [int(x) for x in out_P[:d]]
            else:
                st.P = []
            st._refresh_views()
        self._cpp_dirty = False

    def _sync_py_to_cpp(self, *, keep_py_dirty: bool = False) -> None:
        if self._cpp_eng is None or self._cpp_lib is None:
            return
        if not self._py_dirty and (self._cpp_dirty or len(self._active_shots) == 0):
            return
        lib = self._cpp_lib
        lib.engine_clear(self._cpp_eng)
        for b, st in self._active_shots.items():
            st._sync_external_if_needed()
            d = st._d
            in_A = np.ascontiguousarray(st._A_buf[: max(1, d)], dtype=np.uint64)
            in_ell = np.ascontiguousarray(st._ell_buf[: max(1, d)], dtype=np.uint8)
            in_Gamma = np.ascontiguousarray(
                st._Gamma_buf[: max(1, d), : max(1, d)], dtype=np.uint8
            )
            in_P = np.ascontiguousarray(
                st.P if d > 0 else [-1], dtype=np.int32
            )
            lib.engine_import_shot(
                self._cpp_eng,
                int(b),
                int(d),
                st.h_words.ctypes.data_as(u64_p),
                st.a_union.ctypes.data_as(u64_p),
                in_A.ctypes.data_as(u64_p),
                in_ell.ctypes.data_as(u8_p),
                in_Gamma.ctypes.data_as(u8_p),
                in_P.ctypes.data_as(i32_p),
            )
        self._py_dirty = keep_py_dirty
        self._cpp_dirty = False

    def _init_rngs(self) -> None:
        if self.seed is not None and self.sync_tableside_rng:
            base_seed = int(self.seed) + self._batches_completed * self.batch_size
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
        else:
            self._shot_np_rngs = None
            self._tab_rngs = None
            self._tab_rngs_x = None
            base_seed = (
                None
                if self.seed is None
                else int(self.seed) + self._batches_completed
            )
            self.np_rng = np.random.default_rng(seed=base_seed)

        tie_seed = (
            int(self.np_rng.integers(0, 1 << 63, dtype=np.uint64))
            if base_seed is None
            else int(base_seed)
        )
        b_arange = np.arange(self.batch_size, dtype=np.uint64)
        base_hash = np.uint64(
            ((tie_seed + 1) * 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        )
        self._shot_tiebreak_rng = np.ascontiguousarray(
            base_hash ^ (b_arange * np.uint64(0xBF58476D1CE4E5B9)),
            dtype=np.uint64,
        )
        if getattr(self, "_cpp_eng", None) is not None and getattr(self, "_cpp_lib", None) is not None:
            self._cpp_lib.engine_set_rng_states(
                self._cpp_eng, self._shot_tiebreak_rng.ctypes.data_as(u64_p)
            )

    def _draw_tab_rng_bit(self, b_idx: int) -> bool:
        if self._tab_rngs is not None and self._tab_rngs_x is not None:
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
        if self._ext_rand_bits is not None:
            c = int(self._ext_cursor_arr[b_idx])
            self._ext_cursor_arr[b_idx] = c + 1
            return bool(self._ext_rand_bits[b_idx, c])
        z = (
            int(self._shot_tiebreak_rng[b_idx]) + 0x9E3779B97F4A7C15
        ) & 0xFFFFFFFFFFFFFFFF
        self._shot_tiebreak_rng[b_idx] = np.uint64(z)
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
        return bool((z ^ (z >> 31)) & 1)

    def _sample_pauli_noise_via_tab_rng(
        self,
        op_name: str,
        targets_per_shot: Sequence[Sequence[int]],
        gate_args: Sequence[float],
    ) -> None:
        assert self._tab_rngs is not None and self._tab_rngs_x is not None
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
            for q in set(t_b):
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

    ############################################################################
    # Public lifecycle methods
    ############################################################################

    def run(self) -> None:
        """Run the full circuit across all batch_size shots."""
        if self._used_interactively or self._circuit_time != 0:
            raise ValueError(
                "CosetsideSimulator is not in a clean state. "
                "Use .clear() to prepare the simulator for a new run."
            )
        self._do(self.circuit)
        self._finished_running_circuit = True
        self._batches_completed += 1

    def clear(self) -> None:
        """Reset the simulator state for a new batch without recompiling the reference circuit."""
        self._flip_simulator.clear()
        self._active_shots.clear()
        if self._cpp_eng is not None and self._cpp_lib is not None:
            self._cpp_lib.engine_clear(self._cpp_eng)
        self._cpp_dirty = False
        self._py_dirty = False
        self._ext_cursor_arr.fill(0)
        self._measurement_columns.clear()

        self.qubit_coords = {}
        self.qubit_tags = {}
        self.coords_shifts = []

        self._init_rngs()
        self._compiled_op_handler.clear()

        self._circuit_time = 0
        self._step_cursor = 0
        self._used_interactively = False
        self._finished_running_circuit = False

        self._final_measurement_records = None
        self._detector_flips = None
        self._observable_flips = None

    def get_current_circuit_time(self) -> int:
        return self._circuit_time

    def interactive_do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock, /
    ) -> None:
        if self._finished_running_circuit:
            raise RuntimeError(
                "Cannot do more operations after finishing an interactive run."
            )
        self._used_interactively = True
        self._do(this)

    def finish_interactive_run(self) -> None:
        self._finished_running_circuit = True

    ############################################################################
    # Properties & Lookthroughs
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

    def num_qubits_in_new_circuit(self) -> int:
        return self.num_qubits

    def _current_inv_tableau(self) -> stim.Tableau:
        if self._step_cursor == 0:
            return stim.Tableau(self.num_qubits)
        return self._compiled_ref.steps[self._step_cursor - 1].inv_tableau_after

    def _current_packed_words(self) -> PackedSymplecticTables:
        if self._step_cursor == 0:
            return self._compiled_ref._initial_packed_words
        return self._compiled_ref.steps[self._step_cursor - 1]._get_packed_words(
            self._n_words
        )

    def _next_step_meta(self, op: stim.CircuitInstruction) -> StepMeta:
        if self._step_cursor < len(self._compiled_ref.steps):
            step = self._compiled_ref.steps[self._step_cursor]
            if not self._used_interactively or (
                step.op.name == op.name
                and [t.value for t in step.op.targets_copy()]
                == [t.value for t in op.targets_copy()]
            ):
                self._step_cursor += 1
                return step
        step = self._compiled_ref.compile_and_append_instruction(op)
        self._step_cursor = len(self._compiled_ref.steps)
        return step

    ############################################################################
    # Internal Instruction Dispatch
    ############################################################################

    def _do(
        self, this: stim.Circuit | stim.CircuitInstruction | stim.CircuitRepeatBlock
    ) -> None:
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
            raise NotImplementedError(f"Unsupported circuit object: {type(this)}")

    def _do_instruction(self, op: stim.CircuitInstruction) -> None:
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
            return

        if op.name == "SHIFT_COORDS":
            shifts = op.gate_args_copy()
            for i, s in enumerate(shifts):
                if i >= len(self.coords_shifts):
                    self.coords_shifts.append(s)
                else:
                    self.coords_shifts[i] += s
            return

        self._compiled_op_handler.handle_op(op=op, sss=self)
        self._circuit_time += 1

    def _do_bare_instruction(self, op: stim.Circuit | stim.CircuitInstruction) -> None:
        """Execute an unconditional instruction across the reference tape and all B shots."""
        if isinstance(op, stim.Circuit):
            for sub_op in op:
                self._do_bare_instruction(sub_op)
            return

        name = op.name
        if name in ("DETECTOR", "OBSERVABLE_INCLUDE", "TICK", "BARRIER"):
            self._next_step_meta(op)
            return

        gd = stim.gate_data(name)
        if gd.is_reset or gd.produces_measurements:
            step_meta = self._next_step_meta(op)
            if isinstance(step_meta, MeasStepMeta):
                self._execute_meas_step(step_meta, op_override=op)
            return

        if gd.is_unitary:
            self.do_conditional_clifford(op, condition_met_mask=None)
            return

        if gd.is_noisy_gate:
            self.do_conditional_pauli_noise(op, condition_met_mask=None)
            return

        self._next_step_meta(op)

    ############################################################################
    # Phase 1 & Phase 2: Unitary Gates, Inverse Clifford Injection & Pauli Noise
    ############################################################################

    def _apply_unitary_to_frames(self, op: stim.CircuitInstruction) -> None:
        """Apply an ideal unitary Clifford instruction to stim.FlipSimulator in O(1) time (O(0) on MRQF)."""
        if op.name in ("I", "II", "I_ERROR", "II_ERROR"):
            return
        self._flip_simulator.do(op)

    def _evict_healed_shots(
        self,
        inv_tab_or_step: stim.Tableau | StepMeta,
        *,
        xs_base: NDArray[np.bool_] | None = None,
        zs_base: NDArray[np.bool_] | None = None,
        accum_x_mask: NDArray[np.bool_] | None = None,
        accum_z_mask: NDArray[np.bool_] | None = None,
    ) -> tuple[bool, bool]:
        """Evict any active shot with d == 0 and fold D_t(h) = B_t X(h) B_t^dagger into stim.FlipSimulator."""
        healed = [b_idx for b_idx, mrqf in self._active_shots.items() if mrqf.d == 0]
        if not healed:
            return False, False

        nz_b: list[int] = []
        nz_h: list[NDArray[np.uint64]] = []
        for b_idx in healed:
            mrqf = self._active_shots.pop(b_idx)
            if _words_any_nonzero(mrqf.h_words):
                nz_b.append(b_idx)
                nz_h.append(mrqf.h_words)

        if not nz_b:
            return False, False

        nz_b_arr = np.asarray(nz_b, dtype=np.intp)
        if isinstance(inv_tab_or_step, stim.Tableau):
            _, x2z, _, z2z, _, _ = inv_tab_or_step.to_numpy(bit_packed=True)
            n_bytes = z2z.shape[1]
            h_u8_mat = np.stack(
                [h.view(np.uint8)[:n_bytes] for h in nz_h], axis=0
            )
            xor_x = z2z[:, None, 0] & h_u8_mat[None, :, 0]
            xor_z = x2z[:, None, 0] & h_u8_mat[None, :, 0]
            for wb in range(1, n_bytes):
                xor_x ^= z2z[:, None, wb] & h_u8_mat[None, :, wb]
                xor_z ^= x2z[:, None, wb] & h_u8_mat[None, :, wb]
        else:
            x2z_w, z2z_w = inv_tab_or_step.get_evict_words(self._n_words)
            h_w_mat = np.stack(nz_h, axis=0)
            xor_x = z2z_w[:, None, 0] & h_w_mat[None, :, 0]
            xor_z = x2z_w[:, None, 0] & h_w_mat[None, :, 0]
            for wb in range(1, self._n_words):
                xor_x ^= z2z_w[:, None, wb] & h_w_mat[None, :, wb]
                xor_z ^= x2z_w[:, None, wb] & h_w_mat[None, :, wb]
        x_phys_mat = (np.bitwise_count(xor_x) & 1).astype(np.bool_)
        z_phys_mat = (np.bitwise_count(xor_z) & 1).astype(np.bool_)

        any_x = bool(np.any(x_phys_mat))
        any_z = bool(np.any(z_phys_mat))

        if any_x:
            if accum_x_mask is not None:
                accum_x_mask[:, nz_b_arr] ^= x_phys_mat
            else:
                self._scratch_mask.fill(False)
                self._scratch_mask[:, nz_b_arr] = x_phys_mat
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_mask, p=1.0
                )
            if xs_base is not None:
                xs_base[:, nz_b_arr] ^= x_phys_mat

        if any_z:
            if accum_z_mask is not None:
                accum_z_mask[:, nz_b_arr] ^= z_phys_mat
            else:
                self._scratch_mask.fill(False)
                self._scratch_mask[:, nz_b_arr] = z_phys_mat
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_mask, p=1.0
                )
            if zs_base is not None:
                zs_base[:, nz_b_arr] ^= z_phys_mat

        return any_x, any_z

    def _inject_inverse_cocycle_on_shot(
        self,
        b_idx: int,
        cocycle: InverseCliffordCocycle,
        f_x_local: NDArray[np.bool_],
        f_z_local: NDArray[np.bool_],
    ) -> None:
        """Inject E = U_t^dagger on shot b_idx via P1, P2, or PCP."""
        mrqf = self._active_shots.get(b_idx)
        if mrqf is None:
            mrqf = MRQFState(self.num_qubits, self._n_words)
            self._active_shots[b_idx] = mrqf

        for factor in cocycle.factors:
            gamma = (
                int(
                    np.count_nonzero(
                        (factor.local_x & f_z_local)
                        ^ (factor.local_z & f_x_local)
                    )
                )
                & 1
            )
            kappa_shot = (factor.kappa + (gamma << 1)) & 3
            if factor.kind == "PCP":
                assert (
                    factor.local_x2 is not None
                    and factor.local_z2 is not None
                    and factor.a2_words is not None
                    and factor.b2_words is not None
                )
                gamma2 = (
                    int(
                        np.count_nonzero(
                            (factor.local_x2 & f_z_local)
                            ^ (factor.local_z2 & f_x_local)
                        )
                    )
                    & 1
                )
                kappa2_shot = (factor.kappa2 + (gamma2 << 1)) & 3
                mrqf.p2_apply_controlled_pauli(
                    factor.a_words,
                    factor.b_words,
                    kappa_shot,
                    factor.a2_words,
                    factor.b2_words,
                    kappa2_shot,
                )
            elif factor.kind == "P1":
                mrqf.p1_apply_pauli(factor.a_words, factor.b_words, kappa_shot)
            else:
                mrqf.p2_apply_quarter_turn(
                    factor.a_words, factor.b_words, kappa_shot
                )

    def do_conditional_clifford(
        self,
        op: stim.CircuitInstruction,
        condition_met_mask: NDArray[np.bool_] | None = None,
    ) -> None:
        """Execute a conditional Clifford op by running Phase 2 and injecting E = U_t^dagger where ~condition_met_mask."""
        step_meta = self._next_step_meta(op)
        name = op.name
        gd = stim.gate_data(name)
        if gd.is_two_qubit_gate:
            raw_t = op.targets_copy()
            if any(t.qubit_value is None for t in raw_t):
                target_groups = op.target_groups()
                quant_indices: list[int] = []
                for g_idx, (t0, t1) in enumerate(target_groups):
                    if t0.is_measurement_record_target and t1.qubit_value is not None:
                        rec_col = self._measurement_columns[t0.value]
                        active = (
                            rec_col
                            if condition_met_mask is None
                            else (rec_col & condition_met_mask[g_idx])
                        )
                        if np.any(active):
                            q = t1.qubit_value
                            pauli_char = (
                                "X"
                                if name in ("CX", "CNOT", "ZCX")
                                else ("Y" if name in ("CY", "ZCY") else "Z")
                            )
                            self._scratch_mask.fill(False)
                            self._scratch_mask[q, :] = active
                            self._flip_simulator.broadcast_pauli_errors(
                                pauli=pauli_char, mask=self._scratch_mask, p=1.0
                            )
                    elif t1.is_measurement_record_target and t0.qubit_value is not None:
                        rec_col = self._measurement_columns[t1.value]
                        active = (
                            rec_col
                            if condition_met_mask is None
                            else (rec_col & condition_met_mask[g_idx])
                        )
                        if np.any(active):
                            q = t0.qubit_value
                            pauli_char = (
                                "X"
                                if name == "XCZ"
                                else ("Y" if name == "YCZ" else "Z")
                            )
                            self._scratch_mask.fill(False)
                            self._scratch_mask[q, :] = active
                            self._flip_simulator.broadcast_pauli_errors(
                                pauli=pauli_char, mask=self._scratch_mask, p=1.0
                            )
                    elif t0.qubit_value is not None and t1.qubit_value is not None:
                        quant_indices.append(g_idx)
                if not isinstance(step_meta, UnitaryStepMeta):
                    return
                op = step_meta.op
                if condition_met_mask is not None:
                    condition_met_mask = condition_met_mask[quant_indices, :]

        if not isinstance(step_meta, UnitaryStepMeta):
            return

        if condition_met_mask is None or np.all(condition_met_mask):
            self._apply_unitary_to_frames(op)
            return

        if name in ("I", "II", "I_ERROR", "II_ERROR"):
            return

        if name in ("X", "Y", "Z"):
            self._apply_unitary_to_frames(op)
            self._scratch_mask.fill(False)
            for g_idx, group in enumerate(step_meta.target_groups):
                q = group[0]
                self._scratch_mask[q, :] ^= ~condition_met_mask[g_idx]
            self._flip_simulator.broadcast_pauli_errors(
                pauli=name, mask=self._scratch_mask, p=1.0
            )
            return

        if name in ("XX", "YY", "ZZ"):
            self._apply_unitary_to_frames(op)
            pauli_char = name[0]
            self._scratch_mask.fill(False)
            for g_idx, group in enumerate(step_meta.target_groups):
                skipped = ~condition_met_mask[g_idx]
                self._scratch_mask[group[0], :] ^= skipped
                self._scratch_mask[group[1], :] ^= skipped
            self._flip_simulator.broadcast_pauli_errors(
                pauli=pauli_char, mask=self._scratch_mask, p=1.0
            )
            return

        if step_meta.are_groups_disjoint:
            if self._use_cpp and self._cpp_lib is not None and self._cpp_eng is not None:
                if self._py_dirty or (not self._cpp_dirty and len(self._active_shots) > 0):
                    self._sync_py_to_cpp()
                self._apply_unitary_to_frames(op)
                lib = self._cpp_lib
                if name in ("CZ", "ZCZ"):
                    if not hasattr(step_meta, "_cpp_q0"):
                        step_meta._cpp_q0 = np.array(
                            [g[0] for g in step_meta.target_groups], dtype=np.int64
                        )
                        step_meta._cpp_q1 = np.array(
                            [g[1] for g in step_meta.target_groups], dtype=np.int64
                        )
                    xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                        output_xs=True, output_zs=False
                    )
                    _, x2z_w, z2x_w, z2z_w, _, z_kappa = step_meta._get_packed_words(
                        self._n_words
                    )
                    self._scratch_x_mask.fill(False)
                    self._scratch_z_mask.fill(False)
                    cm_c = np.ascontiguousarray(condition_met_mask, dtype=np.uint8)
                    xs_c = np.ascontiguousarray(xs_base, dtype=np.uint8)
                    lib.engine_batch_inject_cz(
                        self._cpp_eng,
                        len(step_meta.target_groups),
                        step_meta._cpp_q0.ctypes.data_as(i64_p),
                        step_meta._cpp_q1.ctypes.data_as(i64_p),
                        cm_c.ctypes.data_as(u8_p),
                        xs_c.ctypes.data_as(u8_p),
                        z2x_w.ctypes.data_as(u64_p),
                        z2z_w.ctypes.data_as(u64_p),
                        z_kappa.ctypes.data_as(u8_p),
                        x2z_w.ctypes.data_as(u64_p),
                        self._scratch_x_mask.ctypes.data_as(u8_p),
                        self._scratch_z_mask.ctypes.data_as(u8_p),
                        ctypes.byref(self._any_x_c),
                        ctypes.byref(self._any_z_c),
                    )
                    self._cpp_dirty = True
                    if self._any_x_c.value:
                        self._flip_simulator.broadcast_pauli_errors(
                            pauli="X", mask=self._scratch_x_mask, p=1.0
                        )
                    if self._any_z_c.value:
                        self._flip_simulator.broadcast_pauli_errors(
                            pauli="Z", mask=self._scratch_z_mask, p=1.0
                        )
                    return

                if not hasattr(step_meta, "_cpp_cocycle_arrays"):
                    num_g = len(step_meta.target_groups)
                    arity = len(step_meta.target_groups[0])
                    cocycles = [
                        step_meta.get_inverse_cocycle(
                            g, self.num_qubits, self._n_words
                        )
                        for g in range(num_g)
                    ]
                    n_fac = len(cocycles[0].factors)
                    t_groups = np.array(step_meta.target_groups, dtype=np.int64)
                    f_kinds = np.zeros((num_g, n_fac), dtype=np.uint8)
                    f_lx1 = np.zeros((num_g, n_fac, arity), dtype=np.uint8)
                    f_lz1 = np.zeros((num_g, n_fac, arity), dtype=np.uint8)
                    f_a1 = np.zeros(
                        (num_g, n_fac, self._n_words), dtype=np.uint64
                    )
                    f_b1 = np.zeros(
                        (num_g, n_fac, self._n_words), dtype=np.uint64
                    )
                    f_k1 = np.zeros((num_g, n_fac), dtype=np.uint8)
                    f_lx2 = np.zeros((num_g, n_fac, arity), dtype=np.uint8)
                    f_lz2 = np.zeros((num_g, n_fac, arity), dtype=np.uint8)
                    f_a2 = np.zeros(
                        (num_g, n_fac, self._n_words), dtype=np.uint64
                    )
                    f_b2 = np.zeros(
                        (num_g, n_fac, self._n_words), dtype=np.uint64
                    )
                    f_k2 = np.zeros((num_g, n_fac), dtype=np.uint8)
                    for g_i, coc in enumerate(cocycles):
                        for f_i, fac in enumerate(coc.factors):
                            f_kinds[g_i, f_i] = (
                                0
                                if fac.kind == "P1"
                                else (1 if fac.kind == "P2" else 2)
                            )
                            f_lx1[g_i, f_i] = fac.local_x
                            f_lz1[g_i, f_i] = fac.local_z
                            f_a1[g_i, f_i] = fac.a_words
                            f_b1[g_i, f_i] = fac.b_words
                            f_k1[g_i, f_i] = fac.kappa
                            if fac.kind == "PCP":
                                f_lx2[g_i, f_i] = fac.local_x2
                                f_lz2[g_i, f_i] = fac.local_z2
                                f_a2[g_i, f_i] = fac.a2_words
                                f_b2[g_i, f_i] = fac.b2_words
                                f_k2[g_i, f_i] = fac.kappa2
                    step_meta._cpp_cocycle_arrays = (
                        num_g,
                        arity,
                        n_fac,
                        t_groups,
                        f_kinds,
                        f_lx1,
                        f_lz1,
                        f_a1,
                        f_b1,
                        f_k1,
                        f_lx2,
                        f_lz2,
                        f_a2,
                        f_b2,
                        f_k2,
                    )
                (
                    num_g,
                    arity,
                    n_fac,
                    t_groups,
                    f_kinds,
                    f_lx1,
                    f_lz1,
                    f_a1,
                    f_b1,
                    f_k1,
                    f_lx2,
                    f_lz2,
                    f_a2,
                    f_b2,
                    f_k2,
                ) = step_meta._cpp_cocycle_arrays
                xs_base, zs_base, _, _, _ = self._flip_simulator.to_numpy(
                    output_xs=True, output_zs=True
                )
                x2z_w, z2z_w = step_meta.get_evict_words(self._n_words)
                self._scratch_x_mask.fill(False)
                self._scratch_z_mask.fill(False)
                cm_c = np.ascontiguousarray(condition_met_mask, dtype=np.uint8)
                xs_c = np.ascontiguousarray(xs_base, dtype=np.uint8)
                zs_c = np.ascontiguousarray(zs_base, dtype=np.uint8)
                lib.engine_batch_inject_general_clifford(
                    self._cpp_eng,
                    num_g,
                    arity,
                    n_fac,
                    t_groups.ctypes.data_as(i64_p),
                    cm_c.ctypes.data_as(u8_p),
                    xs_c.ctypes.data_as(u8_p),
                    zs_c.ctypes.data_as(u8_p),
                    f_kinds.ctypes.data_as(u8_p),
                    f_lx1.ctypes.data_as(u8_p),
                    f_lz1.ctypes.data_as(u8_p),
                    f_a1.ctypes.data_as(u64_p),
                    f_b1.ctypes.data_as(u64_p),
                    f_k1.ctypes.data_as(u8_p),
                    f_lx2.ctypes.data_as(u8_p),
                    f_lz2.ctypes.data_as(u8_p),
                    f_a2.ctypes.data_as(u64_p),
                    f_b2.ctypes.data_as(u64_p),
                    f_k2.ctypes.data_as(u8_p),
                    x2z_w.ctypes.data_as(u64_p),
                    z2z_w.ctypes.data_as(u64_p),
                    self._scratch_x_mask.ctypes.data_as(u8_p),
                    self._scratch_z_mask.ctypes.data_as(u8_p),
                    ctypes.byref(self._any_x_c),
                    ctypes.byref(self._any_z_c),
                )
                self._cpp_dirty = True
                if self._any_x_c.value:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="X", mask=self._scratch_x_mask, p=1.0
                    )
                if self._any_z_c.value:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="Z", mask=self._scratch_z_mask, p=1.0
                    )
                return

            self._apply_unitary_to_frames(op)
            if name in ("CZ", "ZCZ"):
                xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                    output_xs=True, output_zs=False
                )
                _, _, z2x_w, z2z_w, _, z_kappa = step_meta._get_packed_words(
                    self._n_words
                )
                failed_g_indices = np.flatnonzero(
                    ~np.all(condition_met_mask, axis=1)
                )
                for g_idx_np in failed_g_indices:
                    g_idx = int(g_idx_np)
                    q0, q1 = step_meta.target_groups[g_idx]
                    aw1, bw1, kap1_base = (
                        z2x_w[q0],
                        z2z_w[q0],
                        int(z_kappa[q0]),
                    )
                    aw2, bw2, kap2_base = (
                        z2x_w[q1],
                        z2z_w[q1],
                        int(z_kappa[q1]),
                    )
                    xs_q0 = xs_base[q0]
                    xs_q1 = xs_base[q1]
                    for b_idx_np in np.flatnonzero(~condition_met_mask[g_idx]):
                        b_idx = int(b_idx_np)
                        mrqf = self._active_shots.get(b_idx)
                        if mrqf is None:
                            mrqf = MRQFState(self.num_qubits, self._n_words)
                            self._active_shots[b_idx] = mrqf
                        kap1 = (kap1_base + (int(xs_q0[b_idx]) << 1)) & 3
                        kap2 = (kap2_base + (int(xs_q1[b_idx]) << 1)) & 3
                        mrqf.p2_apply_controlled_pauli(
                            aw1, bw1, kap1, aw2, bw2, kap2
                        )
                if self._active_shots and any(
                    m.d == 0 for m in self._active_shots.values()
                ):
                    self._evict_healed_shots(
                        step_meta,
                        xs_base=xs_base,
                    )
                return

            xs_base, zs_base, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=True, output_zs=True
            )
            failed_g_indices = np.flatnonzero(~np.all(condition_met_mask, axis=1))
            for g_idx_np in failed_g_indices:
                g_idx = int(g_idx_np)
                group = step_meta.target_groups[g_idx]
                failed_shots = np.flatnonzero(~condition_met_mask[g_idx])
                cocycle = step_meta.get_inverse_cocycle(
                    g_idx, self.num_qubits, self._n_words
                )
                group_arr = np.array(group, dtype=np.intp)
                for b_idx_np in failed_shots:
                    b_idx = int(b_idx_np)
                    f_x_local = xs_base[group_arr, b_idx]
                    f_z_local = zs_base[group_arr, b_idx]
                    self._inject_inverse_cocycle_on_shot(
                        b_idx, cocycle, f_x_local, f_z_local
                    )
            if self._active_shots and any(
                m.d == 0 for m in self._active_shots.values()
            ):
                self._evict_healed_shots(
                    step_meta,
                    xs_base=xs_base,
                    zs_base=zs_base,
                )
            return

        if self._use_cpp and self._cpp_dirty:
            self._sync_cpp_to_py()
        for g_idx, group in enumerate(step_meta.target_groups):
            self._apply_unitary_to_frames(step_meta.group_ops[g_idx])
            failed_shots = np.where(~condition_met_mask[g_idx])[0]
            if len(failed_shots) == 0:
                continue
            cocycle = step_meta.get_inverse_cocycle(
                g_idx, self.num_qubits, self._n_words
            )
            xs_base, zs_base, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=True, output_zs=True
            )
            group_arr = np.array(group, dtype=np.intp)
            for b_idx_np in failed_shots:
                b_idx = int(b_idx_np)
                f_x_local = xs_base[group_arr, b_idx]
                f_z_local = zs_base[group_arr, b_idx]
                self._inject_inverse_cocycle_on_shot(
                    b_idx, cocycle, f_x_local, f_z_local
                )
            if self._active_shots:
                self._evict_healed_shots(
                    step_meta.group_inv_tableaus[g_idx],
                    xs_base=xs_base,
                    zs_base=zs_base,
                )
        if self._use_cpp:
            self._py_dirty = True

    def do_conditional_pauli_noise(
        self,
        op: stim.CircuitInstruction,
        condition_met_mask: NDArray[np.bool_] | None = None,
    ) -> None:
        """Apply a (possibly conditioned) Pauli noise channel directly into stim.FlipSimulator in O(1) time."""
        self._next_step_meta(op)
        name = op.name
        if name in ("I", "II", "I_ERROR", "II_ERROR"):
            return

        args = op.gate_args_copy()
        if name in ("E", "CORRELATED_ERROR", "ELSE_CORRELATED_ERROR"):
            if not hasattr(self, "_correlated_error_occurred") or len(
                self._correlated_error_occurred
            ) != self.batch_size:
                self._correlated_error_occurred = np.zeros(
                    self.batch_size, dtype=np.bool_
                )
            p_err = float(args[0]) if len(args) > 0 else 0.0
            raw_targets = op.targets_copy()
            if self._tab_rngs is not None and self._tab_rngs_x is not None:
                self._scratch_x_mask.fill(False)
                self._scratch_z_mask.fill(False)
                any_x = False
                any_z = False
                for b_idx in range(self.batch_size):
                    t_b = (
                        raw_targets
                        if condition_met_mask is None
                        else [
                            raw_targets[i]
                            for i in range(len(raw_targets))
                            if condition_met_mask[i, b_idx]
                        ]
                    )
                    inst_b = stim.CircuitInstruction(name, t_b, [p_err])
                    tr_z = self._tab_rngs[b_idx]
                    tr_x = self._tab_rngs_x[b_idx]
                    tr_z.do(inst_b)
                    tr_x.do(inst_b)
                    for q in {
                        gt.qubit_value
                        for gt in t_b
                        if gt.qubit_value is not None
                    }:
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

            if p_err <= 0.0:
                if name in ("E", "CORRELATED_ERROR"):
                    self._correlated_error_occurred.fill(False)
                return
            u = self.np_rng.random(self.batch_size)
            if name in ("E", "CORRELATED_ERROR"):
                occurred = u < p_err
                self._correlated_error_occurred[:] = occurred
            else:
                occurred = (~self._correlated_error_occurred) & (u < p_err)
                self._correlated_error_occurred |= occurred
            if np.any(occurred) and len(raw_targets) > 0:
                self._scratch_x_mask.fill(False)
                self._scratch_z_mask.fill(False)
                any_x = False
                any_z = False
                for t_idx, gt in enumerate(raw_targets):
                    q = gt.qubit_value
                    if q is None:
                        continue
                    hit = (
                        occurred
                        if condition_met_mask is None
                        else (occurred & condition_met_mask[t_idx])
                    )
                    if not np.any(hit):
                        continue
                    if gt.is_x_target or gt.is_y_target:
                        self._scratch_x_mask[q, :] ^= hit
                        any_x = True
                    if gt.is_z_target or gt.is_y_target:
                        self._scratch_z_mask[q, :] ^= hit
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

        if len(args) == 0 or max(args) <= 0.0:
            return
        p_err = args[0]

        targets = [t.qubit_value for t in op.targets_copy()]
        if len(targets) == 0:
            return

        if self._tab_rngs is not None:
            is_two_qubit = name in ("DEPOLARIZE2", "PAULI_CHANNEL_2")
            if condition_met_mask is None:
                targets_per_shot: list[list[int]] = [
                    targets for _ in range(self.batch_size)
                ]
            elif not is_two_qubit:
                targets_per_shot = [
                    [
                        targets[i]
                        for i in range(len(targets))
                        if condition_met_mask[i, b]
                    ]
                    for b in range(self.batch_size)
                ]
            else:
                pairs = [
                    (targets[i], targets[i + 1])
                    for i in range(0, len(targets), 2)
                ]
                targets_per_shot = []
                for b in range(self.batch_size):
                    flat_b: list[int] = []
                    for g_idx, (q1, q2) in enumerate(pairs):
                        if condition_met_mask[g_idx, b]:
                            flat_b.append(q1)
                            flat_b.append(q2)
                    targets_per_shot.append(flat_b)
            self._sample_pauli_noise_via_tab_rng(name, targets_per_shot, args)
            return

        if condition_met_mask is None or np.all(condition_met_mask):
            self._flip_simulator.do(op)
            return

        if name in ("X_ERROR", "Y_ERROR", "Z_ERROR"):
            pauli_char = name[0]
            self._scratch_mask.fill(False)
            for t_idx, q in enumerate(targets):
                if condition_met_mask is None:
                    self._scratch_mask[q, :] = True
                else:
                    self._scratch_mask[q, :] |= condition_met_mask[t_idx]
            self._flip_simulator.broadcast_pauli_errors(
                pauli=pauli_char, mask=self._scratch_mask, p=p_err
            )
            return

        if name in ("DEPOLARIZE1", "PAULI_CHANNEL_1"):
            num_t = len(targets)
            u = self.np_rng.random((num_t, self.batch_size))
            if condition_met_mask is not None:
                u = np.where(condition_met_mask, u, 1.0)
            if name == "DEPOLARIZE1":
                px = py = pz = p_err / 3.0
            else:
                px, py, pz = args[0], args[1], args[2]
            x_flips = u < (px + py)
            z_flips = (u >= px) & (u < (px + py + pz))
            if np.any(x_flips):
                self._scratch_mask.fill(False)
                for t_idx, q in enumerate(targets):
                    self._scratch_mask[q, :] ^= x_flips[t_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_mask, p=1.0
                )
            if np.any(z_flips):
                self._scratch_mask.fill(False)
                for t_idx, q in enumerate(targets):
                    self._scratch_mask[q, :] ^= z_flips[t_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_mask, p=1.0
                )
            return

        if name == "DEPOLARIZE2":
            pairs = [
                (targets[i], targets[i + 1]) for i in range(0, len(targets), 2)
            ]
            num_p = len(pairs)
            u = self.np_rng.random((num_p, self.batch_size))
            if condition_met_mask is not None:
                u = np.where(condition_met_mask, u, 1.0)
            bucket = np.floor(u / (p_err / 15.0)).astype(np.int32) + 1
            bucket[u >= p_err] = 0
            x1 = (bucket & 1) != 0
            z1 = (bucket & 2) != 0
            x2 = (bucket & 4) != 0
            z2 = (bucket & 8) != 0
            if np.any(x1) or np.any(x2):
                self._scratch_mask.fill(False)
                for g_idx, (q1, q2) in enumerate(pairs):
                    self._scratch_mask[q1, :] ^= x1[g_idx]
                    self._scratch_mask[q2, :] ^= x2[g_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_mask, p=1.0
                )
            if np.any(z1) or np.any(z2):
                self._scratch_mask.fill(False)
                for g_idx, (q1, q2) in enumerate(pairs):
                    self._scratch_mask[q1, :] ^= z1[g_idx]
                    self._scratch_mask[q2, :] ^= z2[g_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_mask, p=1.0
                )
            return

        if name == "PAULI_CHANNEL_2":
            pairs = [
                (targets[i], targets[i + 1]) for i in range(0, len(targets), 2)
            ]
            num_p = len(pairs)
            u = self.np_rng.random((num_p, self.batch_size))
            if condition_met_mask is not None:
                u = np.where(condition_met_mask, u, 1.0)
            cdf = np.cumsum(args)
            idx_sampled = np.searchsorted(cdf, u, side="right")
            p1_code = np.where(idx_sampled < 15, (idx_sampled + 1) // 4, 0)
            p2_code = np.where(idx_sampled < 15, (idx_sampled + 1) % 4, 0)
            x1 = (p1_code == 1) | (p1_code == 2)
            z1 = (p1_code == 2) | (p1_code == 3)
            x2 = (p2_code == 1) | (p2_code == 2)
            z2 = (p2_code == 2) | (p2_code == 3)
            if np.any(x1) or np.any(x2):
                self._scratch_mask.fill(False)
                for g_idx, (q1, q2) in enumerate(pairs):
                    self._scratch_mask[q1, :] ^= x1[g_idx]
                    self._scratch_mask[q2, :] ^= x2[g_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_mask, p=1.0
                )
            if np.any(z1) or np.any(z2):
                self._scratch_mask.fill(False)
                for g_idx, (q1, q2) in enumerate(pairs):
                    self._scratch_mask[q1, :] ^= z1[g_idx]
                    self._scratch_mask[q2, :] ^= z2[g_idx]
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_mask, p=1.0
                )
            return

        if condition_met_mask is None or np.all(condition_met_mask):
            self._flip_simulator.do(op)

    ############################################################################
    # Phase 3 & Phase 4: Projective Measurements & Mid-Circuit Resets via P2/P3
    ############################################################################

    def _execute_meas_step(
        self,
        step_meta: MeasStepMeta,
        op_override: stim.CircuitInstruction | None = None,
    ) -> None:
        """Execute a compiled measurement or reset step across all B shots using P2 and P3."""
        substeps = step_meta.substeps

        if self._use_cpp and self._cpp_lib is not None and self._cpp_eng is not None:
            if self._py_dirty or (not self._cpp_dirty and len(self._active_shots) > 0):
                self._sync_py_to_cpp()
            lib = self._cpp_lib
            ext_bits_p = (
                self._ext_rand_bits.ctypes.data_as(u8_p)
                if self._ext_rand_bits is not None
                else ctypes.cast(0, u8_p)
            )
            ext_cur_p = (
                self._ext_cursor_arr.ctypes.data_as(int_p)
                if self._ext_rand_bits is not None
                else ctypes.cast(0, int_p)
            )

            if (
                len(substeps) > 1
                and not any(s.pre_ops or s.post_ops for s in substeps)
                and all(not s.meta.is_random for s in substeps)
                and len({s.meta.q for s in substeps}) == len(substeps)
            ):
                num_sub = len(substeps)
                q_arr, m_ref_arr, b_sub_words = step_meta.get_det_batch_arrays()
                if not hasattr(step_meta, "_cpp_q_i64"):
                    step_meta._cpp_q_i64 = np.ascontiguousarray(
                        q_arr, dtype=np.int64
                    )
                    step_meta._cpp_mref_u8 = np.ascontiguousarray(
                        m_ref_arr, dtype=np.uint8
                    )
                    step_meta._cpp_bsub_u64 = np.ascontiguousarray(
                        b_sub_words, dtype=np.uint64
                    )
                    step_meta._cpp_m_mat = np.empty(
                        (num_sub, self.batch_size), dtype=np.bool_
                    )
                elif step_meta._cpp_m_mat.shape[1] != self.batch_size:
                    step_meta._cpp_m_mat = np.empty(
                        (num_sub, self.batch_size), dtype=np.bool_
                    )

                xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                    output_xs=True, output_zs=False
                )
                xs_c = np.ascontiguousarray(xs_base, dtype=np.uint8)
                m_mat = step_meta._cpp_m_mat
                x2z_w, z2z_w = step_meta.get_evict_words(self._n_words)
                self._scratch_x_mask.fill(False)
                self._scratch_z_mask.fill(False)

                lib.engine_batch_det_meas_step(
                    self._cpp_eng,
                    num_sub,
                    step_meta._cpp_q_i64.ctypes.data_as(i64_p),
                    step_meta._cpp_mref_u8.ctypes.data_as(u8_p),
                    step_meta._cpp_bsub_u64.ctypes.data_as(u64_p),
                    xs_c.ctypes.data_as(u8_p),
                    m_mat.ctypes.data_as(u8_p),
                    x2z_w.ctypes.data_as(u64_p),
                    z2z_w.ctypes.data_as(u64_p),
                    self._scratch_x_mask.ctypes.data_as(u8_p),
                    self._scratch_z_mask.ctypes.data_as(u8_p),
                    ctypes.byref(self._any_x_c),
                    ctypes.byref(self._any_z_c),
                    ext_bits_p,
                    ext_cur_p,
                )
                self._cpp_dirty = True
                any_x_accum = bool(self._any_x_c.value)
                any_z_accum = bool(self._any_z_c.value)

                if step_meta.produces_measurements:
                    for k_idx, substep in enumerate(substeps):
                        self._measurement_columns.append(
                            ~m_mat[k_idx].copy()
                            if substep.invert_result
                            else m_mat[k_idx].copy()
                        )

                if step_meta.is_reset:
                    for substep in substeps:
                        if substep.meta.reset_flip_ref:
                            self._scratch_x_mask[substep.meta.q, :] ^= True
                            any_x_accum = True
                    kick_mat = m_mat ^ m_ref_arr[:, None]
                    if np.any(kick_mat):
                        self._scratch_x_mask[q_arr, :] ^= kick_mat
                        any_x_accum = True

                if any_x_accum:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="X", mask=self._scratch_x_mask, p=1.0
                    )
                if any_z_accum:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="Z", mask=self._scratch_z_mask, p=1.0
                    )

                if step_meta.produces_measurements:
                    self._apply_measurement_flip_noise(
                        step_meta, op_override, num_sub
                    )
                return

            xs_c: NDArray[np.uint8] | None = None
            self._scratch_x_mask.fill(False)
            self._scratch_z_mask.fill(False)
            self._any_x_c.value = 0
            self._any_z_c.value = 0

            def _flush_cpp_accum() -> None:
                if self._any_x_c.value:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="X", mask=self._scratch_x_mask, p=1.0
                    )
                    self._scratch_x_mask.fill(False)
                    self._any_x_c.value = 0
                if self._any_z_c.value:
                    self._flip_simulator.broadcast_pauli_errors(
                        pauli="Z", mask=self._scratch_z_mask, p=1.0
                    )
                    self._scratch_z_mask.fill(False)
                    self._any_z_c.value = 0

            null_u64 = ctypes.cast(0, u64_p)
            null_i64 = ctypes.cast(0, i64_p)

            for substep in substeps:
                if substep.pre_ops:
                    _flush_cpp_accum()
                    for pre_op in substep.pre_ops:
                        self._apply_unitary_to_frames(pre_op)
                    xs_c = None
                if xs_c is None:
                    xs_base_tmp, _, _, _, _ = self._flip_simulator.to_numpy(
                        output_xs=True, output_zs=False
                    )
                    xs_c = np.ascontiguousarray(xs_base_tmp, dtype=np.uint8)

                meta = substep.meta
                q = meta.q
                m_ref = int(meta.m_ref)
                if not meta.is_random:
                    m_shots = np.empty(self.batch_size, dtype=np.bool_)
                    lib.engine_batch_mixed_substep(
                        self._cpp_eng,
                        0,
                        q,
                        m_ref,
                        0,
                        meta.beta_Z_words.ctypes.data_as(u64_p),
                        null_u64,
                        null_u64,
                        0,
                        0,
                        null_i64,
                        0,
                        null_i64,
                        xs_c.ctypes.data_as(u8_p),
                        m_shots.ctypes.data_as(u8_p),
                        self._scratch_x_mask.ctypes.data_as(u8_p),
                        self._scratch_z_mask.ctypes.data_as(u8_p),
                        ctypes.byref(self._any_x_c),
                        ctypes.byref(self._any_z_c),
                        ext_bits_p,
                        ext_cur_p,
                    )
                else:
                    m_shots = self.np_rng.integers(
                        0, 2, size=self.batch_size, dtype=np.bool_
                    )
                    if not hasattr(meta, "_cpp_gx_i64"):
                        meta._cpp_gx_i64 = np.ascontiguousarray(
                            meta.G_x_indices, dtype=np.int64
                        )
                        meta._cpp_gz_i64 = np.ascontiguousarray(
                            meta.G_z_indices, dtype=np.int64
                        )
                    lib.engine_batch_mixed_substep(
                        self._cpp_eng,
                        1,
                        q,
                        m_ref,
                        meta.p,
                        meta.a_words.ctypes.data_as(u64_p),
                        meta.b_rot_words.ctypes.data_as(u64_p),
                        meta.e_p_words.ctypes.data_as(u64_p),
                        meta.kappa_rot,
                        len(meta._cpp_gx_i64),
                        meta._cpp_gx_i64.ctypes.data_as(i64_p),
                        len(meta._cpp_gz_i64),
                        meta._cpp_gz_i64.ctypes.data_as(i64_p),
                        xs_c.ctypes.data_as(u8_p),
                        m_shots.ctypes.data_as(u8_p),
                        self._scratch_x_mask.ctypes.data_as(u8_p),
                        self._scratch_z_mask.ctypes.data_as(u8_p),
                        ctypes.byref(self._any_x_c),
                        ctypes.byref(self._any_z_c),
                        ext_bits_p,
                        ext_cur_p,
                    )
                self._cpp_dirty = True

                if step_meta.produces_measurements:
                    self._measurement_columns.append(
                        ~m_shots if substep.invert_result else m_shots.copy()
                    )

                if step_meta.is_reset:
                    if meta.reset_flip_ref:
                        self._scratch_x_mask[q, :] ^= True
                        xs_c[q, :] ^= 1
                        self._any_x_c.value = 1
                    kick_shots = m_shots ^ bool(m_ref)
                    if np.any(kick_shots):
                        self._scratch_x_mask[q, :] ^= kick_shots
                        xs_c[q, :] ^= kick_shots.view(np.uint8)
                        self._any_x_c.value = 1

                if substep.post_ops:
                    _flush_cpp_accum()
                    for post_op in substep.post_ops:
                        self._apply_unitary_to_frames(post_op)
                    xs_c = None

            x2z_w, z2z_w = step_meta.get_evict_words(self._n_words)
            ev_x_c = ctypes.c_int(0)
            ev_z_c = ctypes.c_int(0)
            lib.engine_batch_evict(
                self._cpp_eng,
                x2z_w.ctypes.data_as(u64_p),
                z2z_w.ctypes.data_as(u64_p),
                xs_c.ctypes.data_as(u8_p)
                if xs_c is not None
                else ctypes.cast(0, u8_p),
                ctypes.cast(0, u8_p),
                self._scratch_x_mask.ctypes.data_as(u8_p),
                self._scratch_z_mask.ctypes.data_as(u8_p),
                ctypes.byref(ev_x_c),
                ctypes.byref(ev_z_c),
            )
            if ev_x_c.value:
                self._any_x_c.value = 1
            if ev_z_c.value:
                self._any_z_c.value = 1
            _flush_cpp_accum()
            if step_meta.produces_measurements:
                self._apply_measurement_flip_noise(
                    step_meta, op_override, len(substeps)
                )
            return

        xs_base: NDArray[np.bool_] | None = None
        self._scratch_x_mask.fill(False)
        self._scratch_z_mask.fill(False)
        any_x_accum = False
        any_z_accum = False

        def _flush_accum() -> None:
            nonlocal any_x_accum, any_z_accum
            if any_x_accum:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_x_mask, p=1.0
                )
                self._scratch_x_mask.fill(False)
                any_x_accum = False
            if any_z_accum:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_z_mask, p=1.0
                )
                self._scratch_z_mask.fill(False)
                any_z_accum = False

        if (
            len(substeps) > 1
            and not any(s.pre_ops or s.post_ops for s in substeps)
            and all(not s.meta.is_random for s in substeps)
            and len({s.meta.q for s in substeps}) == len(substeps)
        ):
            num_sub = len(substeps)
            q_arr, m_ref_arr, b_sub_words = step_meta.get_det_batch_arrays()
            xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=True, output_zs=False
            )
            m_mat = xs_base[q_arr, :] ^ m_ref_arr[:, None]

            if self._active_shots:
                active_indices = list(self._active_shots.keys())
                active_mrqfs = [self._active_shots[b] for b in active_indices]
                h_mat = np.stack([m.h_words for m in active_mrqfs], axis=0)
                au_mat = np.stack([m.a_union for m in active_mrqfs], axis=0)
                xor_bh = b_sub_words[:, None, 0] & h_mat[None, :, 0]
                ov_words = b_sub_words[:, None, 0] & au_mat[None, :, 0]
                for w in range(1, self._n_words):
                    xor_bh ^= b_sub_words[:, None, w] & h_mat[None, :, w]
                    ov_words |= b_sub_words[:, None, w] & au_mat[None, :, w]
                bh_mat = (np.bitwise_count(xor_bh) & 1).astype(np.bool_)
                m_mat[:, active_indices] ^= bh_mat
                overlap_mat = ov_words != 0
                if np.any(overlap_mat):
                    for a_idx_np in np.flatnonzero(np.any(overlap_mat, axis=0)):
                        a_idx = int(a_idx_np)
                        b_idx = active_indices[a_idx]
                        mrqf = active_mrqfs[a_idx]
                        if mrqf.d == 0:
                            continue
                        h_init = mrqf.h_words.copy()
                        for k_np in np.flatnonzero(overlap_mat[:, a_idx]):
                            k_idx = int(k_np)
                            b_k = b_sub_words[k_idx]
                            if mrqf.d == 0 or not _words_any_overlap(
                                b_k, mrqf.a_union
                            ):
                                if _word_dot_parity(
                                    b_k, mrqf.h_words ^ h_init
                                ):
                                    m_mat[k_idx, b_idx] = not m_mat[
                                        k_idx, b_idx
                                    ]
                                continue
                            q_k = int(q_arr[k_idx])
                            m_ref_k = int(m_ref_arr[k_idx])
                            f_q = int(xs_base[q_k, b_idx])
                            sigma = (f_q ^ m_ref_k) & 1
                            m_val, _ = mrqf.p3_measure_diagonal(
                                b_k,
                                sigma,
                                draw_bit_fn=lambda b=b_idx: int(
                                    self._draw_tab_rng_bit(b)
                                ),
                            )
                            m_mat[k_idx, b_idx] = bool(m_val)

                ev_x, ev_z = self._evict_healed_shots(
                    step_meta,
                    xs_base=xs_base,
                    accum_x_mask=self._scratch_x_mask,
                    accum_z_mask=self._scratch_z_mask,
                )
                any_x_accum |= ev_x
                any_z_accum |= ev_z

            if step_meta.produces_measurements:
                for k_idx, substep in enumerate(substeps):
                    self._measurement_columns.append(
                        ~m_mat[k_idx]
                        if substep.invert_result
                        else m_mat[k_idx]
                    )

            if step_meta.is_reset:
                for substep in substeps:
                    if substep.meta.reset_flip_ref:
                        self._scratch_x_mask[substep.meta.q, :] ^= True
                        any_x_accum = True
                kick_mat = m_mat ^ m_ref_arr[:, None]
                if np.any(kick_mat):
                    self._scratch_x_mask[q_arr, :] ^= kick_mat
                    any_x_accum = True

            _flush_accum()
            if step_meta.produces_measurements:
                self._apply_measurement_flip_noise(
                    step_meta, op_override, num_sub
                )
            return

        active_indices_arr: NDArray[np.intp] | None = None
        active_mrqfs_list: list[MRQFState] = []
        h_mat_mixed: NDArray[np.uint64] | None = None
        au_mat_mixed: NDArray[np.uint64] | None = None
        if self._active_shots and self._tab_rngs is None:
            active_indices_list = list(self._active_shots.keys())
            active_indices_arr = np.asarray(active_indices_list, dtype=np.intp)
            active_mrqfs_list = [
                self._active_shots[b] for b in active_indices_list
            ]
            h_mat_mixed = np.stack(
                [m.h_words for m in active_mrqfs_list], axis=0
            )
            au_mat_mixed = np.stack(
                [m.a_union for m in active_mrqfs_list], axis=0
            )

        for substep in substeps:
            if substep.pre_ops:
                _flush_accum()
                for pre_op in substep.pre_ops:
                    self._apply_unitary_to_frames(pre_op)
                xs_base = None

            meta = substep.meta
            q = meta.q
            m_ref = meta.m_ref
            if xs_base is None:
                xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                    output_xs=True, output_zs=False
                )
            f_q_shots = xs_base[q, :].copy()

            if not meta.is_random:
                m_shots = f_q_shots ^ bool(m_ref)
                if self._active_shots:
                    b_words = meta.beta_Z_words
                    if (
                        active_indices_arr is not None
                        and h_mat_mixed is not None
                        and au_mat_mixed is not None
                    ):
                        xor_bh_sub = h_mat_mixed[:, 0] & b_words[0]
                        ov_sub = au_mat_mixed[:, 0] & b_words[0]
                        for w in range(1, self._n_words):
                            xor_bh_sub ^= h_mat_mixed[:, w] & b_words[w]
                            ov_sub |= au_mat_mixed[:, w] & b_words[w]
                        m_shots[active_indices_arr] ^= (
                            np.bitwise_count(xor_bh_sub) & 1
                        ).astype(np.bool_)
                        for a_idx_np in np.flatnonzero(ov_sub != 0):
                            a_idx = int(a_idx_np)
                            b_idx = int(active_indices_arr[a_idx])
                            mrqf = active_mrqfs_list[a_idx]
                            sigma = (int(f_q_shots[b_idx]) ^ int(m_ref)) & 1
                            m_val, _ = mrqf.p3_measure_diagonal(
                                b_words,
                                sigma,
                                draw_bit_fn=lambda b=b_idx: int(
                                    self._draw_tab_rng_bit(b)
                                ),
                            )
                            m_shots[b_idx] = bool(m_val)
                            h_mat_mixed[a_idx] = mrqf.h_words
                            au_mat_mixed[a_idx] = mrqf.a_union
                    else:
                        for b_idx, mrqf in self._active_shots.items():
                            if mrqf.d == 0 or not _words_any_overlap(
                                b_words, mrqf.a_union
                            ):
                                if _word_dot_parity(b_words, mrqf.h_words):
                                    m_shots[b_idx] = not m_shots[b_idx]
                            else:
                                sigma = (int(f_q_shots[b_idx]) ^ int(m_ref)) & 1
                                m_val, _ = mrqf.p3_measure_diagonal(
                                    b_words,
                                    sigma,
                                    draw_bit_fn=lambda b=b_idx: int(
                                        self._draw_tab_rng_bit(b)
                                    ),
                                )
                                m_shots[b_idx] = bool(m_val)
            else:
                p = meta.p
                p_word = p >> 6
                p_bit = p & 63
                p_mask = np.uint64(1 << p_bit)
                if self._tab_rngs is not None:
                    m_shots = np.zeros(self.batch_size, dtype=np.bool_)
                    delta_shots = np.zeros(self.batch_size, dtype=np.bool_)
                    for b_idx in range(self.batch_size):
                        mrqf = self._active_shots.get(b_idx)
                        if mrqf is not None:
                            if not bool(mrqf.a_union[p_word] & p_mask):
                                m_b = self._draw_tab_rng_bit(b_idx)
                                m_shots[b_idx] = m_b
                                h_p = bool(
                                    (int(mrqf.h_words[p_word]) >> p_bit) & 1
                                )
                                delta_shots[b_idx] = (
                                    m_b
                                    ^ bool(f_q_shots[b_idx])
                                    ^ h_p
                                    ^ bool(m_ref)
                                )
                            else:
                                f_q_b = int(f_q_shots[b_idx])
                                mrqf.p2_apply_quarter_turn(
                                    meta.a_words,
                                    meta.b_rot_words,
                                    meta.kappa_rot,
                                )
                                m_eff, _ = mrqf.p3_measure_diagonal(
                                    meta.e_p_words,
                                    0,
                                    draw_bit_fn=lambda b=b_idx, fq=f_q_b: int(
                                        self._draw_tab_rng_bit(b)
                                    )
                                    ^ fq,
                                )
                                m_shots[b_idx] = bool(m_eff ^ f_q_b)
                                delta_shots[b_idx] = False
                        else:
                            m_shots[b_idx] = self._draw_tab_rng_bit(b_idx)
                            delta_shots[b_idx] = (
                                m_shots[b_idx] ^ f_q_shots[b_idx] ^ bool(m_ref)
                            )
                else:
                    m_shots = self.np_rng.integers(
                        0, 2, size=self.batch_size, dtype=np.bool_
                    )
                    delta_shots = m_shots ^ f_q_shots ^ bool(m_ref)
                    if (
                        active_indices_arr is not None
                        and h_mat_mixed is not None
                        and au_mat_mixed is not None
                    ):
                        delta_shots[active_indices_arr] ^= (
                            h_mat_mixed[:, p_word] & p_mask
                        ) != 0
                        for a_idx_np in np.flatnonzero(
                            (au_mat_mixed[:, p_word] & p_mask) != 0
                        ):
                            a_idx = int(a_idx_np)
                            b_idx = int(active_indices_arr[a_idx])
                            mrqf = active_mrqfs_list[a_idx]
                            f_q_b = int(f_q_shots[b_idx])
                            delta_shots[b_idx] = False
                            mrqf.p2_apply_quarter_turn(
                                meta.a_words,
                                meta.b_rot_words,
                                meta.kappa_rot,
                            )
                            m_eff, _ = mrqf.p3_measure_diagonal(
                                meta.e_p_words,
                                0,
                                draw_bit_fn=lambda b=b_idx, fq=f_q_b: int(
                                    m_shots[b]
                                )
                                ^ fq,
                            )
                            m_shots[b_idx] = bool(m_eff ^ f_q_b)
                            h_mat_mixed[a_idx] = mrqf.h_words
                            au_mat_mixed[a_idx] = mrqf.a_union

                if np.any(delta_shots):
                    if len(meta.G_x_indices) > 0:
                        self._scratch_x_mask[meta.G_x_indices, :] ^= (
                            delta_shots[None, :]
                        )
                        xs_base[meta.G_x_indices, :] ^= delta_shots[None, :]
                        any_x_accum = True
                    if len(meta.G_z_indices) > 0:
                        self._scratch_z_mask[meta.G_z_indices, :] ^= (
                            delta_shots[None, :]
                        )
                        any_z_accum = True

            if step_meta.produces_measurements:
                recorded_shots = (
                    ~m_shots if substep.invert_result else m_shots.copy()
                )
                self._measurement_columns.append(recorded_shots)

            if step_meta.is_reset:
                if meta.reset_flip_ref:
                    self._scratch_x_mask[q, :] ^= True
                    xs_base[q, :] ^= True
                    any_x_accum = True
                kick_shots = m_shots ^ bool(m_ref)
                if np.any(kick_shots):
                    self._scratch_x_mask[q, :] ^= kick_shots
                    xs_base[q, :] ^= kick_shots
                    any_x_accum = True

            if substep.post_ops:
                _flush_accum()
                for post_op in substep.post_ops:
                    self._apply_unitary_to_frames(post_op)
                xs_base = None

        if self._active_shots:
            ev_x, ev_z = self._evict_healed_shots(
                step_meta,
                xs_base=xs_base,
                accum_x_mask=self._scratch_x_mask,
                accum_z_mask=self._scratch_z_mask,
            )
            any_x_accum |= ev_x
            any_z_accum |= ev_z

        _flush_accum()

        if step_meta.produces_measurements:
            self._apply_measurement_flip_noise(
                step_meta, op_override, len(step_meta.substeps)
            )

    def _apply_measurement_flip_noise(
        self,
        step_meta: MeasStepMeta,
        op_override: stim.CircuitInstruction | None,
        num_m: int,
    ) -> None:
        args = (
            op_override.gate_args_copy()
            if op_override is not None
            else step_meta.op.gate_args_copy()
        )
        if len(args) > 0 and args[0] > 0.0:
            p_meas_err = args[0]
            if self._tab_rngs is not None and self._tab_rngs_x is not None:
                mpad_op = stim.CircuitInstruction(
                    "MPAD", [0] * num_m, [p_meas_err]
                )
                for b_idx in range(self.batch_size):
                    tr = self._tab_rngs[b_idx]
                    tr.do(mpad_op)
                    self._tab_rngs_x[b_idx].do(mpad_op)
                    rec = tr.current_measurement_record()
                    flips = rec[-num_m:]
                    for k_idx, flipped in enumerate(flips):
                        if flipped:
                            self._measurement_columns[-num_m + k_idx][
                                b_idx
                            ] = not self._measurement_columns[-num_m + k_idx][
                                b_idx
                            ]
            else:
                flips_2d = (
                    self.np_rng.random((num_m, self.batch_size)) < p_meas_err
                )
                for k_idx in range(num_m):
                    self._measurement_columns[-num_m + k_idx] ^= flips_2d[
                        k_idx
                    ]

    ############################################################################
    # Unmatched Mid-Circuit Noisy Resets (e.g. Leakage Transitions 2 -> 0 / 1)
    ############################################################################

    def do_unmatched_noisy_reset_z(
        self, reset_zero_mask: NDArray[np.bool_], reset_one_mask: NDArray[np.bool_]
    ) -> None:
        """Reset noisy qubits to |0> or |1> where reset_zero_mask or reset_one_mask is True without altering the reference tableau."""
        if not (np.any(reset_zero_mask) or np.any(reset_one_mask)):
            return

        _, x2z_w, z2x_w, z2z_w, _, z_kappa = self._current_packed_words()

        if self._use_cpp and self._cpp_lib is not None and self._cpp_eng is not None:
            if self._py_dirty or (not self._cpp_dirty and len(self._active_shots) > 0):
                self._sync_py_to_cpp()
            xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
                output_xs=True, output_zs=False
            )
            xs_c = np.ascontiguousarray(xs_base, dtype=np.uint8)
            r0_c = np.ascontiguousarray(reset_zero_mask, dtype=np.uint8)
            r1_c = np.ascontiguousarray(reset_one_mask, dtype=np.uint8)
            self._scratch_x_mask.fill(False)
            self._scratch_z_mask.fill(False)
            self._any_x_c.value = 0
            self._any_z_c.value = 0

            self._cpp_lib.engine_batch_unmatched_noisy_reset_z(
                self._cpp_eng,
                r0_c.ctypes.data_as(u8_p),
                r1_c.ctypes.data_as(u8_p),
                xs_c.ctypes.data_as(u8_p),
                z2x_w.ctypes.data_as(u64_p),
                z2z_w.ctypes.data_as(u64_p),
                z_kappa.ctypes.data_as(u8_p),
                x2z_w.ctypes.data_as(u64_p),
                self._scratch_x_mask.ctypes.data_as(u8_p),
                self._scratch_z_mask.ctypes.data_as(u8_p),
                ctypes.byref(self._any_x_c),
                ctypes.byref(self._any_z_c),
            )
            self._cpp_dirty = True
            if self._any_x_c.value:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=self._scratch_x_mask, p=1.0
                )
            if self._any_z_c.value:
                self._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=self._scratch_z_mask, p=1.0
                )
            return

        combined_mask = reset_zero_mask | reset_one_mask
        affected_qubits = np.flatnonzero(np.any(combined_mask, axis=1))
        if len(affected_qubits) == 0:
            return

        xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
            output_xs=True, output_zs=False
        )
        self._scratch_x_mask.fill(False)
        self._scratch_z_mask.fill(False)
        any_x_kick = False
        any_z_kick = False

        for q_np in affected_qubits:
            q = int(q_np)
            shots_for_q = np.flatnonzero(combined_mask[q])
            if len(shots_for_q) == 0:
                continue

            aw = z2x_w[q]
            bw = z2z_w[q]
            kap = int(z_kappa[q])
            a_is_zero = not _words_any_nonzero(aw)
            ab_pop = int(np.bitwise_count(aw & bw).sum())
            sign_bool = int(((kap - ab_pop) & 3) == 2)
            ref_exp = -1 if sign_bool else 1

            b_rot_words: NDArray[np.uint64] | None = None
            e_p_words: NDArray[np.uint64] | None = None
            kappa_rot: int = 0
            if not a_is_zero:
                p = _first_set_bit_in_words(aw)
                kappa_rot = (kap + 1) & 3
                b_rot_words = bw.copy()
                b_rot_words[p >> 6] ^= np.uint64(1 << (p & 63))
                e_p_words = np.zeros(self._n_words, dtype=np.uint64)
                e_p_words[p >> 6] = np.uint64(1 << (p & 63))

            for b_np in shots_for_q:
                b_idx = int(b_np)
                target_state = 1 if reset_one_mask[q, b_idx] else 0
                f_q = int(xs_base[q, b_idx])
                mrqf = self._active_shots.get(b_idx)
                if mrqf is not None and (
                    mrqf.d > 0 or _words_any_nonzero(mrqf.h_words)
                ):
                    mu = mrqf.q1_pauli_expectation(aw, bw, kap)
                    z_exp = -mu if f_q else mu
                else:
                    z_exp = (-ref_exp if f_q else ref_exp) if a_is_zero else 0

                if z_exp == 1:
                    if target_state == 1:
                        self._scratch_x_mask[q, b_idx] ^= True
                        xs_base[q, b_idx] ^= True
                        any_x_kick = True
                elif z_exp == -1:
                    if target_state == 0:
                        self._scratch_x_mask[q, b_idx] ^= True
                        xs_base[q, b_idx] ^= True
                        any_x_kick = True
                else:
                    m = int(self._draw_tab_rng_bit(b_idx))
                    m_eff = (m ^ f_q) & 1
                    if mrqf is None:
                        mrqf = MRQFState(self.num_qubits, self._n_words)
                        self._active_shots[b_idx] = mrqf
                    if a_is_zero:
                        sigma = (f_q ^ sign_bool) & 1
                        mrqf.p3_measure_diagonal(bw, sigma, requested_bit=m)
                    else:
                        assert b_rot_words is not None and e_p_words is not None
                        mrqf.p2_apply_quarter_turn(aw, b_rot_words, kappa_rot)
                        mrqf.p3_measure_diagonal(
                            e_p_words, 0, requested_bit=m_eff
                        )
                        mrqf.p2_apply_quarter_turn(
                            aw, b_rot_words, (kappa_rot + 2) & 3
                        )
                    if m != target_state:
                        self._scratch_x_mask[q, b_idx] ^= True
                        xs_base[q, b_idx] ^= True
                        any_x_kick = True
                    if mrqf.d == 0:
                        self._active_shots.pop(b_idx, None)
                        if _words_any_nonzero(mrqf.h_words):
                            hw = mrqf.h_words
                            for qq in range(self.num_qubits):
                                if _word_dot_parity(z2z_w[qq], hw):
                                    self._scratch_x_mask[qq, b_idx] ^= True
                                    xs_base[qq, b_idx] ^= True
                                    any_x_kick = True
                                if _word_dot_parity(x2z_w[qq], hw):
                                    self._scratch_z_mask[qq, b_idx] ^= True
                                    any_z_kick = True

        if any_z_kick:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=self._scratch_z_mask, p=1.0
            )
        if any_x_kick:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=self._scratch_x_mask, p=1.0
            )

    def postselect_z_batch(
        self,
        q: int,
        flip_mask: NDArray[np.bool_],
        desired_bits: NDArray[np.uint8],
    ) -> None:
        """Project Z_q onto (-1)^desired_bits[b] for shots where flip_mask[b] is True."""
        shots_for_q = np.flatnonzero(flip_mask)
        if len(shots_for_q) == 0:
            return

        if self._use_cpp and self._cpp_dirty:
            self._sync_cpp_to_py()

        _, x2z_w, z2x_w, z2z_w, _, z_kappa = self._current_packed_words()
        xs_base, _, _, _, _ = self._flip_simulator.to_numpy(
            output_xs=True, output_zs=False
        )
        self._scratch_x_mask.fill(False)
        self._scratch_z_mask.fill(False)
        any_x_kick = False
        any_z_kick = False

        aw = z2x_w[q]
        bw = z2z_w[q]
        kap = int(z_kappa[q])
        a_is_zero = not _words_any_nonzero(aw)
        ab_pop = int(np.bitwise_count(aw & bw).sum())
        sign_bool = int(((kap - ab_pop) & 3) == 2)

        b_rot_words: NDArray[np.uint64] | None = None
        e_p_words: NDArray[np.uint64] | None = None
        kappa_rot: int = 0
        if not a_is_zero:
            p = _first_set_bit_in_words(aw)
            kappa_rot = (kap + 1) & 3
            b_rot_words = bw.copy()
            b_rot_words[p >> 6] ^= np.uint64(1 << (p & 63))
            e_p_words = np.zeros(self._n_words, dtype=np.uint64)
            e_p_words[p >> 6] = np.uint64(1 << (p & 63))

        for b_np in shots_for_q:
            b_idx = int(b_np)
            m = int(desired_bits[b_idx]) & 1
            f_q = int(xs_base[q, b_idx])
            m_eff = (m ^ f_q) & 1
            mrqf = self._active_shots.get(b_idx)
            if mrqf is None:
                mrqf = MRQFState(self.num_qubits, self._n_words)
                self._active_shots[b_idx] = mrqf
            if a_is_zero:
                sigma = (f_q ^ sign_bool) & 1
                mrqf.p3_measure_diagonal(bw, sigma, requested_bit=m)
            else:
                assert b_rot_words is not None and e_p_words is not None
                mrqf.p2_apply_quarter_turn(aw, b_rot_words, kappa_rot)
                mrqf.p3_measure_diagonal(
                    e_p_words, 0, requested_bit=m_eff
                )
                mrqf.p2_apply_quarter_turn(
                    aw, b_rot_words, (kappa_rot + 2) & 3
                )
            if mrqf.d == 0:
                self._active_shots.pop(b_idx, None)
                if _words_any_nonzero(mrqf.h_words):
                    hw = mrqf.h_words
                    for qq in range(self.num_qubits):
                        if _word_dot_parity(z2z_w[qq], hw):
                            self._scratch_x_mask[qq, b_idx] ^= True
                            xs_base[qq, b_idx] ^= True
                            any_x_kick = True
                        if _word_dot_parity(x2z_w[qq], hw):
                            self._scratch_z_mask[qq, b_idx] ^= True
                            any_z_kick = True

        self._py_dirty = True
        if any_z_kick:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=self._scratch_z_mask, p=1.0
            )
        if any_x_kick:
            self._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=self._scratch_x_mask, p=1.0
            )

    def record_direct_measurements(self, outcomes_per_target: NDArray[np.bool_]) -> None:
        """Append externally sampled measurement outcomes (shape: (num_targets, batch_size)), e.g. for MPAD[LEAKAGE_MEASUREMENT]."""
        for t_idx in range(outcomes_per_target.shape[0]):
            self._measurement_columns.append(
                outcomes_per_target[t_idx].astype(np.bool_)
            )

    ############################################################################
    # Observable Expectation & State Peeking via Q1 (PauliExpectation)
    ############################################################################

    def peek_pauli_batch(
        self, targets: Sequence[int] | NDArray[np.int_], pauli: Literal["X", "Y", "Z"]
    ) -> NDArray[np.int8]:
        """Return exact single-qubit Pauli expectations (+1, -1, or 0) of shape (len(targets), batch_size) via Q1."""
        targets_arr = np.ascontiguousarray(targets, dtype=np.int64)
        if len(targets_arr) == 0:
            return np.zeros((0, self.batch_size), dtype=np.int8)

        x2x_w, x2z_w, z2x_w, z2z_w, x_kappa, z_kappa = (
            self._current_packed_words()
        )
        xs_base, zs_base, _, _, _ = self._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )

        if self._use_cpp and self._cpp_lib is not None and self._cpp_eng is not None:
            if self._py_dirty or (not self._cpp_dirty and len(self._active_shots) > 0):
                self._sync_py_to_cpp(keep_py_dirty=self._py_dirty)
            xs_c = np.ascontiguousarray(xs_base, dtype=np.uint8)
            zs_c = np.ascontiguousarray(zs_base, dtype=np.uint8)
            out = np.empty((len(targets_arr), self.batch_size), dtype=np.int8)
            p_kind = 2 if pauli == "Z" else (0 if pauli == "X" else 1)
            self._cpp_lib.engine_peek_pauli_batch(
                self._cpp_eng,
                len(targets_arr),
                targets_arr.ctypes.data_as(i64_p),
                p_kind,
                xs_c.ctypes.data_as(u8_p),
                zs_c.ctypes.data_as(u8_p),
                x2x_w.ctypes.data_as(u64_p),
                x2z_w.ctypes.data_as(u64_p),
                z2x_w.ctypes.data_as(u64_p),
                z2z_w.ctypes.data_as(u64_p),
                x_kappa.ctypes.data_as(u8_p),
                z_kappa.ctypes.data_as(u8_p),
                out.ctypes.data_as(i8_p),
            )
            return out

        if pauli == "Z":
            f_comm_mat = xs_base[targets_arr, :]
            a_words_all = z2x_w[targets_arr]
            b_words_all = z2z_w[targets_arr]
            kappas = z_kappa[targets_arr]
        elif pauli == "X":
            f_comm_mat = zs_base[targets_arr, :]
            a_words_all = x2x_w[targets_arr]
            b_words_all = x2z_w[targets_arr]
            kappas = x_kappa[targets_arr]
        elif pauli == "Y":
            f_comm_mat = xs_base[targets_arr, :] ^ zs_base[targets_arr, :]
            a_words_all = x2x_w[targets_arr] ^ z2x_w[targets_arr]
            b_words_all = x2z_w[targets_arr] ^ z2z_w[targets_arr]
            dp = (
                np.bitwise_count(
                    x2z_w[targets_arr] & z2x_w[targets_arr]
                ).sum(axis=1, dtype=np.int32)
                & 1
            )
            kappas = (
                (
                    1
                    + x_kappa[targets_arr].astype(np.int32)
                    + z_kappa[targets_arr].astype(np.int32)
                    + (dp << 1)
                )
                & 3
            ).astype(np.uint8)
        else:
            raise ValueError(f"Unrecognised Pauli: {pauli}")

        a_is_zero = ~np.any(a_words_all != 0, axis=1)
        signs_bool = (kappas & 3) == 2
        ref_exp = np.where(signs_bool, -1, 1).astype(np.int8)
        out = np.where(
            a_is_zero[:, None],
            np.where(f_comm_mat, -ref_exp[:, None], ref_exp[:, None]),
            np.int8(0),
        )

        if self._active_shots:
            num_t = len(targets_arr)
            for idx_t in range(num_t):
                aw = a_words_all[idx_t]
                bw = b_words_all[idx_t]
                kap = int(kappas[idx_t])
                f_c_row = f_comm_mat[idx_t]
                for b_idx, mrqf in self._active_shots.items():
                    mu = mrqf.q1_pauli_expectation(aw, bw, kap)
                    out[idx_t, b_idx] = -mu if f_c_row[b_idx] else mu

        return out

    def peek_z(self, target: int) -> int | NDArray[np.int8]:
        res = self.peek_pauli_batch([target], "Z")[0]
        if self.batch_size == 1 or np.all(res == res[0]):
            return int(res[0])
        return res

    def peek_x(self, target: int) -> int | NDArray[np.int8]:
        res = self.peek_pauli_batch([target], "X")[0]
        if self.batch_size == 1 or np.all(res == res[0]):
            return int(res[0])
        return res

    def peek_y(self, target: int) -> int | NDArray[np.int8]:
        res = self.peek_pauli_batch([target], "Y")[0]
        if self.batch_size == 1 or np.all(res == res[0]):
            return int(res[0])
        return res

    def get_current_noisy_tableau_pauli_state(
        self, targets: Iterable[int], pauli: Literal["X", "Y", "Z"]
    ) -> list[int] | NDArray[np.int8]:
        targets_list = list(targets)
        batch_res = self.peek_pauli_batch(targets_list, pauli)
        if self.batch_size == 1:
            return [int(x) for x in batch_res[:, 0]]
        return batch_res

    ############################################################################
    # Readout Records & Detector/Observable Conversion
    ############################################################################

    def current_tableau_measurement_record(self) -> NDArray[np.bool_]:
        if len(self._measurement_columns) == 0:
            return np.zeros((0,), dtype=np.bool_)
        records = np.column_stack(self._measurement_columns)
        return records[0] if self.batch_size == 1 else records

    def get_final_measurement_records(self) -> NDArray[np.bool_]:
        if not self._finished_running_circuit:
            raise RuntimeError("The circuit has not been fully run yet.")
        if self._final_measurement_records is None:
            if len(self._measurement_columns) == 0:
                self._final_measurement_records = np.zeros(
                    (self.batch_size, 0), dtype=np.bool_
                )
            else:
                self._final_measurement_records = np.column_stack(
                    self._measurement_columns
                )
        return self._final_measurement_records

    def _convert_measurements_to_detector_flips(self) -> None:
        records = self.get_final_measurement_records()
        self._detector_flips, self._observable_flips = (
            self._compiled_m2d_converter.convert(
                measurements=records,
                separate_observables=True,
            )
        )

    def get_detector_flips(self, append_observables: bool = False) -> NDArray[np.bool_]:
        if self._detector_flips is None:
            self._convert_measurements_to_detector_flips()
        assert self._detector_flips is not None
        if append_observables and self._observable_flips is not None:
            return np.hstack([self._detector_flips, self._observable_flips])
        return self._detector_flips

    def get_observable_flips(self) -> NDArray[np.bool_]:
        if self._observable_flips is None:
            self._convert_measurements_to_detector_flips()
        assert self._observable_flips is not None
        return self._observable_flips
