"""Base class of the per-shot DEM generators accepted by the stimside samplers."""

from __future__ import annotations

import abc
from typing import Any, Sequence

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.util.leakage_events import ShotLeakageEvents


class DemGenerator(abc.ABC):
    """A per-shot DEM generator (e.g. the ``dem_gen`` of ``MarginalDecoder``).

    Called with a circuit and its ``(shots, measurements)`` measurement records
    (or one shot's ``(measurements,)`` record), it returns the DEM(s) to decode
    those shots with.

    Args:
        unconditional_condition_on_U: must match the op handler's flag of the
            same name. It decides whether untagged gates skip leaked qubits.
        decompose_errors: default for ``__call__``/``base_dem``: decompose the
            DEM into matchable graphlike components.
        reweight_only: default for ``__call__``: produce only the per-shot
            reweight updates instead of full DEMs.
        loss_oracle: build each shot's DEM from the shot's true leakage events
            instead of its measurement records (see ``needs_leakage_events``).
    """

    def __init__(
        self,
        unconditional_condition_on_U: bool = True,
        decompose_errors: bool = False,
        reweight_only: bool = False,
        loss_oracle: bool = False,
    ) -> None:
        self.unconditional_condition_on_U = unconditional_condition_on_U
        self.decompose_errors = bool(decompose_errors)
        self.reweight_only = bool(reweight_only)
        self.loss_oracle = bool(loss_oracle)

    @property
    def needs_leakage_events(self) -> bool:
        """Whether ``__call__`` needs ``leakage_events`` (the simulator must record them)."""
        return self.loss_oracle

    @abc.abstractmethod
    def base_dem(
        self, circuit: stim.Circuit, *, decompose_errors: bool | None = None
    ) -> stim.DetectorErrorModel:
        """The DEM of ``circuit`` for a shot without leakage."""

    @abc.abstractmethod
    def __call__(
        self,
        circuit: stim.Circuit,
        records: NDArray[np.bool_],
        decompose_errors: bool | None = None,
        reweight_only: bool | None = None,
        *,
        leakage_events: Sequence[ShotLeakageEvents] | ShotLeakageEvents | None = None,
    ) -> Any:
        """Per-shot DEMs (or reweight updates) for the shots of ``records``.

        ``leakage_events`` holds one ``ShotLeakageEvents`` per shot of 2D
        ``records`` (a single shot's events for 1D ``records``); it is required
        when ``needs_leakage_events`` and ignored otherwise.
        """
