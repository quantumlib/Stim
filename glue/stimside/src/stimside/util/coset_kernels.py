from __future__ import annotations

import ctypes
from pathlib import Path

from stimside.util._kernel_loader import (
    find_prebuilt_so as _find_prebuilt_so_candidate,
    load_cdll,
    resolve_so,
)

u64_p = ctypes.POINTER(ctypes.c_uint64)
u8_p = ctypes.POINTER(ctypes.c_uint8)
i8_p = ctypes.POINTER(ctypes.c_int8)
i32_p = ctypes.POINTER(ctypes.c_int32)
i64_p = ctypes.POINTER(ctypes.c_int64)
int_p = ctypes.POINTER(ctypes.c_int)
void_p = ctypes.c_void_p

_LIB_INSTANCE: ctypes.CDLL | None = None


def _resolve_coset_kernels_so(force_recompile: bool = False) -> Path:
    return resolve_so(__file__, "coset_kernels", force_recompile, _find_prebuilt_so_candidate)


def get_coset_kernels_lib() -> ctypes.CDLL:
    """Locate (or compile if needed) and return the cached ctypes CDLL for coset_kernels.cpp."""
    global _LIB_INSTANCE
    if _LIB_INSTANCE is not None:
        return _LIB_INSTANCE

    lib = load_cdll(_resolve_coset_kernels_so)

    lib.engine_create.argtypes = [
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_uint64,
        u64_p,
    ]
    lib.engine_create.restype = void_p

    lib.engine_destroy.argtypes = [void_p]
    lib.engine_destroy.restype = None

    lib.engine_set_rng_states.argtypes = [void_p, u64_p]
    lib.engine_set_rng_states.restype = None

    lib.engine_clear.argtypes = [void_p]
    lib.engine_clear.restype = None

    lib.engine_get_active_shot_indices.argtypes = [void_p, i32_p, i32_p]
    lib.engine_get_active_shot_indices.restype = ctypes.c_int

    lib.engine_export_shot.argtypes = [
        void_p,
        ctypes.c_int,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i32_p,
    ]
    lib.engine_export_shot.restype = None

    lib.engine_import_shot.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i32_p,
    ]
    lib.engine_import_shot.restype = None

    lib.engine_batch_inject_cz.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        i64_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_inject_cz.restype = None

    lib.engine_batch_inject_general_clifford.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        i64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_inject_general_clifford.restype = None

    lib.engine_batch_det_meas_step.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_det_meas_step.restype = None

    lib.engine_batch_mixed_substep.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int64,
        ctypes.c_uint8,
        ctypes.c_int64,
        u64_p,
        u64_p,
        u64_p,
        ctypes.c_int,
        ctypes.c_int,
        i64_p,
        ctypes.c_int,
        i64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_mixed_substep.restype = None

    lib.engine_batch_evict.argtypes = [
        void_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_evict.restype = None

    lib.engine_batch_unmatched_noisy_reset_z.argtypes = [
        void_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_unmatched_noisy_reset_z.restype = None

    lib.engine_peek_pauli_batch.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        ctypes.c_int,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i8_p,
    ]
    lib.engine_peek_pauli_batch.restype = None

    _LIB_INSTANCE = lib
    return lib
