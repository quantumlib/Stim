"""Leakage decoders: the ``dem_decoder`` passed to the stimside samplers.

A ``LeakageDecoder`` holds only settings (so it pickles to sinter's worker
processes). ``compile_for_task(task)`` is called once per compiled sampler with
the task's circuit and returns a ``CompiledLeakageDecoder`` that decodes each
batch from its detection events (and, if ``needs_records`` /
``needs_leakage_events``, the shots' measurement records / leakage events).
"""

from __future__ import annotations

import abc
from typing import Any, Callable, Protocol, Sequence, Union

import numpy as np
from numpy.typing import NDArray
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators.dem_decoding import (
    _decompose_dem_graphlike,
    _warn_if_pymatching_hyperedge,
    decode_with_generated_dems,
)
from stimside.dem_generators.dem_generator_base import DemGenerator
from stimside.dem_generators.dem_generator_marginal import MarginalLeakageDemGenerator
from stimside.util.leakage_events import ShotLeakageEvents


def _call_dem_gen(
    dem_gen: Callable[..., Any],
    circuit: stim.Circuit,
    records: NDArray[np.bool_],
    decompose_errors: bool,
    reweight_only: bool,
    leakage_events: Sequence[ShotLeakageEvents] | None = None,
) -> Any:
    if isinstance(dem_gen, DemGenerator) and (
        decompose_errors or reweight_only or dem_gen.needs_leakage_events
    ):
        extra: dict[str, Any] = {}
        if dem_gen.needs_leakage_events:
            extra["leakage_events"] = leakage_events
        return dem_gen(
            circuit,
            records,
            decompose_errors=decompose_errors or dem_gen.decompose_errors,
            reweight_only=reweight_only or dem_gen.reweight_only,
            **extra,
        )
    return dem_gen(circuit, records)


class CompiledLeakageDecoder(abc.ABC):
    """A ``LeakageDecoder`` compiled for one circuit."""

    @abc.abstractmethod
    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: NDArray[np.uint8],
        records: NDArray[np.bool_] | None = None,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> NDArray[np.uint8]:
        """Predicted observable flips, ``uint8[shots, ceil(num_obs / 8)]``.

        Args:
            bit_packed_detection_event_data: ``uint8[shots, ceil(num_dets / 8)]``,
                little-endian bit-packed detection events.
            records: ``bool[shots, num_measurements]`` measurement records of the
                same shots; required when the decoder ``needs_records``.
            leakage_events: one ``ShotLeakageEvents`` per shot; required when the
                decoder ``needs_leakage_events``.
        """


class LeakageDecoder(abc.ABC):
    """Abstract base of the ``dem_decoder`` accepted by the stimside samplers.

    The samplers always pass the simulator's detection events, plus the
    measurement records only if ``needs_records`` and the leakage events only
    if ``needs_leakage_events``. Both flags must not depend on the task; the
    samplers read them when compiling for a task.
    ``needs_records`` is a settable attribute: a class default (False here)
    that an instance may override, e.g. ``dec.needs_records = True``. Leave it
    False when records are not used: fetching them makes Tableside with
    ``batch_size > 1`` sample full measurement records instead of only the
    detection events.
    ``name`` identifies the decoder; with ``sinter.collect`` use it as the
    task's ``decoder`` and as the ``custom_decoders`` key of the sampler.
    """

    needs_records: bool = False

    @property
    @abc.abstractmethod
    def name(self) -> str:
        """Name of this decoder (e.g. for ``sinter.Task.decoder``)."""

    @property
    def needs_leakage_events(self) -> bool:
        return False

    @abc.abstractmethod
    def compile_for_task(self, task: sinter.Task) -> CompiledLeakageDecoder:
        """Compile for ``task.circuit`` (called once per compiled sampler)."""


class DecoderCallable(Protocol):
    """A custom decoder ``f(circuit, bit_packed_detection_events, records=None)``.

    Called once per batch with the task's circuit, the batch's
    ``uint8[shots, ceil(num_dets / 8)]`` bit-packed detection events and, as the
    keyword ``records``, its ``bool[shots, num_measurements]`` measurement
    records, or ``None`` (the samplers pass them iff the wrapping
    ``CallableDecoder`` ``needs_records``, True by default); returns the
    predicted observable flips, ``uint8[shots, ceil(num_obs / 8)]``.
    """

    def __call__(
        self,
        circuit: stim.Circuit,
        bit_packed_detection_events: NDArray[np.uint8],
        /,
        records: NDArray[np.bool_] | None = None,
    ) -> NDArray[np.uint8]: ...

LeakageDecoderLike = Union[LeakageDecoder, DecoderCallable]


def as_leakage_decoder(dem_decoder: object, owner: str) -> LeakageDecoder:
    """Returns ``dem_decoder`` as a ``LeakageDecoder``.

    A ``LeakageDecoder`` is returned as is; any other callable is taken to be a
    ``DecoderCallable`` and wrapped in ``CallableDecoder(dem_decoder)`` (which
    ``needs_records``). Raises ``TypeError`` otherwise (``owner`` names the
    caller in the message).
    """
    if isinstance(dem_decoder, LeakageDecoder):
        return dem_decoder
    # DemGenerators return DEMs, not predictions; classes are not decoders.
    if callable(dem_decoder) and not isinstance(dem_decoder, (type, DemGenerator)):
        return CallableDecoder(dem_decoder)
    raise TypeError(
        f"{owner} requires dem_decoder to be a LeakageDecoder (e.g. "
        f"MarginalDecoder(...) or BaseDecoder(...)) or a callable "
        f"f(circuit, bit_packed_detection_events, records=None) -> bit-packed "
        f"predictions, got {type(dem_decoder).__name__}."
    )


def _check_decoder(decoder: sinter.Decoder | str, owner: str) -> str:
    """Validates a ``decoder`` setting; returns its name for the default name."""
    if decoder is None:
        raise ValueError(f"{owner} requires a decoder.")
    if isinstance(decoder, str) and decoder not in sinter.BUILT_IN_DECODERS:
        raise ValueError(f"Unknown built-in sinter decoder: {decoder!r}.")
    return decoder if isinstance(decoder, str) else type(decoder).__name__


class BaseDecoder(LeakageDecoder):
    """Decodes every shot with one static DEM and a decoder.

    ``needs_records`` is False (the DEM is static); setting it True makes the
    samplers fetch and pass the records, which this class ignores.

    Args:
        dem: the DEM to decode with. ``None`` uses the task's
            ``detector_error_model`` if set, else the circuit's DEM.
        decoder: a ``sinter.Decoder`` or the name of one in
            ``sinter.BUILT_IN_DECODERS``.
        decompose_errors: decompose the DEM into graphlike components.
        name: the decoder's name; defaults to ``"base:<decoder>"``.
    """

    def __init__(
        self,
        dem: stim.DetectorErrorModel | None = None,
        decoder: sinter.Decoder | str = "pymatching",
        decompose_errors: bool = False,
        name: str | None = None,
    ) -> None:
        if dem is not None and not isinstance(dem, stim.DetectorErrorModel):
            raise TypeError(
                "BaseDecoder requires dem to be a stim.DetectorErrorModel or None, "
                f"got {type(dem).__name__}."
            )
        dec_name = _check_decoder(decoder, "BaseDecoder")
        self.dem = dem
        self.decoder = decoder
        self.decompose_errors = decompose_errors
        self._name = f"base:{dec_name}" if name is None else name

    @property
    def name(self) -> str:
        return self._name

    def compile_for_task(self, task: sinter.Task) -> CompiledLeakageDecoder:
        if task.circuit is None:
            raise ValueError("BaseDecoder requires a circuit in the task.")
        decoder = self.decoder
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]
        dem = self.dem
        if dem is None:
            dem = task.detector_error_model or task.circuit.detector_error_model(
                decompose_errors=self.decompose_errors
            )
        if self.decompose_errors:
            dem = _decompose_dem_graphlike(dem)
        else:
            _warn_if_pymatching_hyperedge(decoder, dem)
        return _CompiledBaseDecoder(decoder.compile_decoder_for_dem(dem=dem))


class _CompiledBaseDecoder(CompiledLeakageDecoder):
    def __init__(self, compiled: sinter.CompiledDecoder) -> None:
        self.compiled = compiled

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: NDArray[np.uint8],
        records: NDArray[np.bool_] | None = None,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> NDArray[np.uint8]:
        return self.compiled.decode_shots_bit_packed(
            bit_packed_detection_event_data=bit_packed_detection_event_data
        )


class CallableDecoder(LeakageDecoder):
    """Adapts a ``DecoderCallable`` to a ``LeakageDecoder``.

    The samplers wrap a bare callable ``dem_decoder`` as ``CallableDecoder(fn)``.

    Args:
        fn: ``f(circuit, bit_packed_detection_events, records=None)``; see
            ``DecoderCallable``. With ``sinter.collect`` it must pickle (e.g. a
            module-level function).
        needs_records: whether the samplers pass the shots' measurement records.
            If False, ``fn`` gets ``records=None`` and Tableside with
            ``batch_size > 1`` samples only the detection events (faster).
        name: the decoder's name; defaults to ``fn.__name__``.
    """

    def __init__(
        self,
        fn: DecoderCallable,
        needs_records: bool = True,
        name: str | None = None,
    ) -> None:
        self.fn = fn
        self.needs_records = needs_records
        self._name = getattr(fn, "__name__", type(fn).__name__) if name is None else name

    @property
    def name(self) -> str:
        return self._name

    def compile_for_task(self, task: sinter.Task) -> CompiledLeakageDecoder:
        if task.circuit is None:
            raise ValueError(f"dem_decoder {self.name} requires a circuit in the task.")
        return _CompiledCallableDecoder(self.fn, task.circuit)


class _CompiledCallableDecoder(CompiledLeakageDecoder):
    def __init__(self, fn: DecoderCallable, circuit: stim.Circuit) -> None:
        self.fn = fn
        self.circuit = circuit

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: NDArray[np.uint8],
        records: NDArray[np.bool_] | None = None,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> NDArray[np.uint8]:
        predictions = np.asarray(
            self.fn(self.circuit, bit_packed_detection_event_data, records=records)
        )
        want = (
            bit_packed_detection_event_data.shape[0],
            (self.circuit.num_observables + 7) // 8,
        )
        if predictions.shape != want:
            raise ValueError(
                f"A callable dem_decoder must return bit-packed predictions of shape "
                f"{want} (shots, ceil(num_observables / 8)), got {predictions.shape}."
            )
        return predictions


class MarginalDecoder(LeakageDecoder):
    """Decodes with the DEMs of a ``MarginalLeakageDemGenerator`` and a decoder.

    Args:
        dem_gen: the generator of each shot's DEM from its measurement records.
            ``None`` uses ``MarginalLeakageDemGenerator()`` (default settings).
            For one static DEM use ``BaseDecoder``.
        decoder: a ``sinter.Decoder`` or the name of one in
            ``sinter.BUILT_IN_DECODERS``.
        decompose_errors: decompose the DEM(s) into graphlike components
            (OR-ed with the generator's own flag).
        reweight_only: decode with per-shot reweight updates (OR-ed with the
            generator's own flag).
        name: the decoder's name; defaults to ``"marginal:<decoder>"``.
    """

    def __init__(
        self,
        dem_gen: MarginalLeakageDemGenerator | None = None,
        decoder: sinter.Decoder | str = "pymatching",
        decompose_errors: bool = False,
        reweight_only: bool = False,
        name: str | None = None,
    ) -> None:
        if dem_gen is None:
            dem_gen = MarginalLeakageDemGenerator()
        if not isinstance(dem_gen, MarginalLeakageDemGenerator):
            raise TypeError(
                "MarginalDecoder requires dem_gen to be a MarginalLeakageDemGenerator "
                f"or None, got {type(dem_gen).__name__}."
            )
        dec_name = _check_decoder(decoder, "MarginalDecoder")
        self.dem_gen = dem_gen
        self.decoder = decoder
        self.decompose_errors = decompose_errors
        self.reweight_only = reweight_only
        self._name = f"marginal:{dec_name}" if name is None else name

    @property
    def name(self) -> str:
        return self._name

    @property
    def needs_records(self) -> bool:
        """Always True: the ``dem_gen`` builds each shot's DEM from its records."""
        return True

    @needs_records.setter
    def needs_records(self, value: bool) -> None:
        if not value:
            raise ValueError(
                "MarginalDecoder needs records (its dem_gen builds each shot's DEM "
                "from them); needs_records cannot be set to False. For decoding "
                "without records use BaseDecoder."
            )

    @property
    def needs_leakage_events(self) -> bool:
        return self.dem_gen.needs_leakage_events

    def compile_for_task(self, task: sinter.Task) -> CompiledLeakageDecoder:
        if task.circuit is None:
            raise ValueError("MarginalDecoder requires a circuit in the task.")
        decoder = self.decoder
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]
        return _CompiledMarginalDecoder(
            circuit=task.circuit,
            decoder=decoder,
            dem_gen=self.dem_gen,
            decompose_errors=self.decompose_errors,
            reweight_only=self.reweight_only,
        )


class _CompiledMarginalDecoder(CompiledLeakageDecoder):
    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        dem_gen: MarginalLeakageDemGenerator,
        decompose_errors: bool,
        reweight_only: bool,
    ) -> None:
        self.circuit = circuit
        self.decoder = decoder
        self.dem_gen = dem_gen
        self.decompose_errors = decompose_errors
        self.reweight_only = reweight_only

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: NDArray[np.uint8],
        records: NDArray[np.bool_] | None = None,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> NDArray[np.uint8]:
        if records is None:
            raise ValueError("MarginalDecoder with a dem_gen requires records.")
        if self.dem_gen.needs_leakage_events and leakage_events is None:
            raise ValueError(
                "MarginalDecoder with a loss_oracle dem_gen requires leakage_events."
            )
        dem = _call_dem_gen(
            self.dem_gen,
            self.circuit,
            records,
            self.decompose_errors,
            self.reweight_only,
            leakage_events=leakage_events,
        )
        return decode_with_generated_dems(
            self.decoder,
            dem,
            bit_packed_detection_event_data,
            decompose_errors=self.decompose_errors,
            reweight_only=self.reweight_only,
        )
