import pickle

import numpy as np
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

import stimside
from stimside.dem_generators import (
    BaseDecoder,
    CallableDecoder,
    LeakageDecoder,
    MarginalDecoder,
    MarginalLeakageDemGenerator,
)
from stimside.dem_generators.dem_decoding import decode_with_generated_dems
from stimside.dem_generators.dem_generator_marginal_test import REP_CIRCUIT
from stimside.dem_generators.leakage_decoder import as_leakage_decoder
from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_flip import FlipsideSampler
from stimside.sampler_tableau import TablesideSampler

# D0 and L0 both equal the random M 0.
COIN_CIRCUIT = stim.Circuit(
    "R 0\nX_ERROR(0.5) 0\nM 0\nDETECTOR rec[-1]\nOBSERVABLE_INCLUDE(0) rec[-1]"
)
FLIPS = stim.DetectorErrorModel("error(0.1) D0 L0")
KEEPS = stim.DetectorErrorModel("error(0.1) D0\nlogical_observable L0")


def _records_decoder(circuit, bit_packed_dets, records=None):
    """Predicts L0 = M 0 from the records: exact iff they are the decoded shots'."""
    assert bit_packed_dets.shape[0] == records.shape[0]
    return np.packbits(records[:, :1], axis=1, bitorder="little")


def _records_and_dets(circuit: stim.Circuit, shots: int = 16):
    records = circuit.compile_sampler(seed=3).sample(shots)
    dets = circuit.compile_m2d_converter().convert(
        measurements=records, append_observables=False
    )
    return records, np.packbits(dets, axis=1, bitorder="little")


def test_exports():
    assert stimside.LeakageDecoder is LeakageDecoder
    assert stimside.MarginalDecoder is MarginalDecoder
    assert stimside.BaseDecoder is BaseDecoder
    assert stimside.CallableDecoder is CallableDecoder
    assert issubclass(MarginalDecoder, LeakageDecoder)
    assert issubclass(BaseDecoder, LeakageDecoder)
    assert issubclass(CallableDecoder, LeakageDecoder)
    with pytest.raises(TypeError):
        LeakageDecoder()  # abstract


@pytest.mark.parametrize(
    "sampler_cls", [TablesideSampler, CosetsideSampler, FlipsideSampler]
)
@pytest.mark.parametrize(
    "bad",
    [
        "pymatching",
        sinter.BUILT_IN_DECODERS["pymatching"],
        None,
        REP_CIRCUIT.detector_error_model(),
        MarginalLeakageDemGenerator(),  # callable, but returns DEMs
        MarginalDecoder,  # the class, not an instance
    ],
)
def test_samplers_reject_non_leakage_decoders(sampler_cls, bad):
    with pytest.raises(TypeError, match="requires dem_decoder to be a LeakageDecoder"):
        sampler_cls(_TrivialOpHandler(), bad)


def test_marginal_decoder_settings():
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    default = MarginalDecoder()
    assert default.name == "marginal:pymatching"
    assert isinstance(default.dem_gen, MarginalLeakageDemGenerator)
    assert not default.dem_gen.decompose_errors and not default.dem_gen.reweight_only
    assert default.needs_records and not default.needs_leakage_events
    assert MarginalDecoder().dem_gen is not default.dem_gen
    weighted = MarginalDecoder(MarginalLeakageDemGenerator())
    assert weighted.needs_records and not weighted.needs_leakage_events
    oracle = MarginalDecoder(gen, name="mine")
    assert oracle.name == "mine"
    assert oracle.needs_records and oracle.needs_leakage_events
    assert (
        MarginalDecoder(decoder=sinter.BUILT_IN_DECODERS["pymatching"]).name
        == f"marginal:{type(sinter.BUILT_IN_DECODERS['pymatching']).__name__}"
    )
    with pytest.raises(TypeError, match="MarginalLeakageDemGenerator"):
        MarginalDecoder(REP_CIRCUIT.detector_error_model())
    with pytest.raises(TypeError, match="MarginalLeakageDemGenerator"):
        MarginalDecoder(lambda circuit, records: None)
    with pytest.raises(ValueError, match="Unknown built-in sinter decoder"):
        MarginalDecoder(decoder="no_such_decoder")
    clone = pickle.loads(pickle.dumps(oracle))
    assert clone.name == "mine" and clone.needs_leakage_events


def test_base_decoder_settings():
    static = BaseDecoder()
    assert static.name == "base:pymatching"
    assert not static.needs_records and not static.needs_leakage_events
    assert BaseDecoder(decoder="vacuous", name="mine").name == "mine"
    assert (
        BaseDecoder(decoder=sinter.BUILT_IN_DECODERS["pymatching"]).name
        == f"base:{type(sinter.BUILT_IN_DECODERS['pymatching']).__name__}"
    )
    with pytest.raises(TypeError, match="stim.DetectorErrorModel"):
        BaseDecoder(MarginalLeakageDemGenerator())
    with pytest.raises(ValueError, match="Unknown built-in sinter decoder"):
        BaseDecoder(decoder="no_such_decoder")
    with pytest.raises(ValueError, match="BaseDecoder requires a decoder"):
        BaseDecoder(decoder=None)
    clone = pickle.loads(pickle.dumps(BaseDecoder(KEEPS, name="k")))
    assert clone.name == "k" and clone.dem == KEEPS


def test_decoders_standalone_decode_matches_decode_with_generated_dems():
    records, dets = _records_and_dets(REP_CIRCUIT)
    task = sinter.Task(circuit=REP_CIRCUIT)
    for decoder, gen in [
        (MarginalDecoder(), MarginalLeakageDemGenerator()),
        (MarginalDecoder(MarginalLeakageDemGenerator()), MarginalLeakageDemGenerator()),
    ]:
        got = decoder.compile_for_task(task).decode_shots_bit_packed(
            bit_packed_detection_event_data=dets, records=records
        )
        want = decode_with_generated_dems("pymatching", gen(REP_CIRCUIT, records), dets)
        np.testing.assert_array_equal(got, want)

    static = BaseDecoder().compile_for_task(task)
    want_static = (
        sinter.BUILT_IN_DECODERS["pymatching"]
        .compile_decoder_for_dem(dem=REP_CIRCUIT.detector_error_model())
        .decode_shots_bit_packed(bit_packed_detection_event_data=dets)
    )
    np.testing.assert_array_equal(
        static.decode_shots_bit_packed(bit_packed_detection_event_data=dets),
        want_static,
    )


def test_base_decoder_dem_precedence():
    # With D0 fired, a DEM whose error flips L0 predicts 1; KEEPS predicts 0.
    dets = np.ones((4, 1), dtype=np.uint8)

    def predict(decoder, **task_kw):
        compiled = decoder.compile_for_task(sinter.Task(circuit=COIN_CIRCUIT, **task_kw))
        return compiled.decode_shots_bit_packed(bit_packed_detection_event_data=dets)

    assert predict(BaseDecoder()).tolist() == [[1]] * 4  # the circuit's DEM
    assert predict(BaseDecoder(), detector_error_model=KEEPS).tolist() == [[0]] * 4
    assert predict(BaseDecoder(KEEPS)).tolist() == [[0]] * 4
    assert predict(BaseDecoder(FLIPS), detector_error_model=KEEPS).tolist() == [[1]] * 4


def test_marginal_decoder_requires_records_and_leakage_events():
    records, dets = _records_and_dets(REP_CIRCUIT)
    task = sinter.Task(circuit=REP_CIRCUIT)
    for decoder in (MarginalDecoder(), MarginalDecoder(MarginalLeakageDemGenerator())):
        compiled = decoder.compile_for_task(task)
        with pytest.raises(ValueError, match="requires records"):
            compiled.decode_shots_bit_packed(bit_packed_detection_event_data=dets)
    oracle = MarginalDecoder(
        MarginalLeakageDemGenerator(loss_oracle=True)
    ).compile_for_task(task)
    with pytest.raises(ValueError, match="requires leakage_events"):
        oracle.decode_shots_bit_packed(
            bit_packed_detection_event_data=dets, records=records
        )


def test_callable_adapter():
    adapted = as_leakage_decoder(_records_decoder, "test")
    assert isinstance(adapted, LeakageDecoder)
    assert adapted.name == "_records_decoder"
    assert adapted.needs_records and not adapted.needs_leakage_events
    decoder = MarginalDecoder()
    assert as_leakage_decoder(decoder, "test") is decoder
    compiled = adapted.compile_for_task(sinter.Task(circuit=COIN_CIRCUIT))
    records, dets = _records_and_dets(COIN_CIRCUIT)
    np.testing.assert_array_equal(
        compiled.decode_shots_bit_packed(
            bit_packed_detection_event_data=dets, records=records
        ),
        np.packbits(records, axis=1, bitorder="little"),
    )
    clone = pickle.loads(pickle.dumps(adapted))
    assert clone.name == "_records_decoder"


def _dets_decoder(circuit, bit_packed_dets, records=None):
    """Predicts L0 = D0 (COIN_CIRCUIT) from the detection events alone."""
    return bit_packed_dets & 1


def test_callable_dem_decoder_records_are_optional():
    got_records = []

    def f(circuit, dets, records=None):
        got_records.append(records)
        return _dets_decoder(circuit, dets)

    compiled = as_leakage_decoder(f, "test").compile_for_task(
        sinter.Task(circuit=COIN_CIRCUIT)
    )
    records, dets = _records_and_dets(COIN_CIRCUIT)
    np.testing.assert_array_equal(
        compiled.decode_shots_bit_packed(bit_packed_detection_event_data=dets),
        np.packbits(records, axis=1, bitorder="little"),
    )
    assert got_records == [None]
    compiled.decode_shots_bit_packed(
        bit_packed_detection_event_data=dets, records=records
    )
    assert got_records[1] is records


def _flat_records_decoder(circuit, bit_packed_dets, records=None):
    return _records_decoder(circuit, bit_packed_dets, records)[:, 0]


def test_callable_adapter_rejects_mis_shaped_predictions():
    # A (shots,) result would broadcast against (shots, 1) and miscount every shot.
    compiled = as_leakage_decoder(_flat_records_decoder, "test").compile_for_task(
        sinter.Task(circuit=COIN_CIRCUIT)
    )
    records, dets = _records_and_dets(COIN_CIRCUIT)
    with pytest.raises(ValueError, match=r"must return bit-packed predictions of shape"):
        compiled.decode_shots_bit_packed(
            bit_packed_detection_event_data=dets, records=records
        )


_CALLABLE_SAMPLERS = {
    "tab": lambda f: TablesideSampler(op_handler=LeakageUint8(), batch_size=4, dem_decoder=f),
    "cos": lambda f: CosetsideSampler(
        op_handler=LeakageUint8Coset(), batch_size=16, dem_decoder=f
    ),
    "flip": lambda f: FlipsideSampler(
        op_handler=LeakageUint8Flip(), batch_size=64, dem_decoder=f
    ),
}


@pytest.mark.parametrize("kind", list(_CALLABLE_SAMPLERS))
def test_samplers_run_a_callable_dem_decoder_on_the_decoded_shots_records(kind):
    calls = []

    def recording(circuit, bit_packed_dets, records=None):
        calls.append(records.shape)
        assert circuit == COIN_CIRCUIT
        return _records_decoder(circuit, bit_packed_dets, records)

    sampler = _CALLABLE_SAMPLERS[kind](recording)
    assert sampler.dem_decoder.name == "recording"
    assert sampler.dem_decoder.needs_records
    stats = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=COIN_CIRCUIT, decoder="recording")
    ).sample(suggested_shots=256)
    assert stats.shots >= 256
    assert stats.errors == 0
    assert calls and all(shape[1] == 1 for shape in calls)


@pytest.mark.parametrize("kind", list(_CALLABLE_SAMPLERS))
def test_samplers_run_a_callable_dem_decoder_that_ignores_records(kind):
    stats = (
        _CALLABLE_SAMPLERS[kind](_dets_decoder)
        .compiled_sampler_for_task(
            sinter.Task(circuit=COIN_CIRCUIT, decoder="_dets_decoder")
        )
        .sample(suggested_shots=256)
    )
    assert stats.shots >= 256
    assert stats.errors == 0


def test_callable_dem_decoder_samplers_pickle_and_run_in_sinter_workers():
    samplers = {k: make(_records_decoder) for k, make in _CALLABLE_SAMPLERS.items()}
    for s in samplers.values():
        assert pickle.loads(pickle.dumps(s)).dem_decoder.name == "_records_decoder"
    stats = sinter.collect(
        num_workers=2,
        tasks=[
            sinter.Task(circuit=COIN_CIRCUIT, decoder=k, json_metadata={"k": k})
            for k in samplers
        ],
        max_shots=512,
        custom_decoders=samplers,
    )
    assert sorted(s.decoder for s in stats) == sorted(samplers)
    for s in stats:
        assert s.shots >= 512 and s.errors == 0, s


def test_callable_decoder_settings_and_pickle():
    default = CallableDecoder(_dets_decoder)
    assert default.name == "_dets_decoder" and default.needs_records
    dec = CallableDecoder(_dets_decoder, needs_records=False, name="dets")
    assert dec.name == "dets" and not dec.needs_records and not dec.needs_leakage_events
    clone = pickle.loads(pickle.dumps(dec))
    assert clone.fn is _dets_decoder and clone.name == "dets" and not clone.needs_records
    assert as_leakage_decoder(dec, "test") is dec


def test_needs_records_instance_override():
    base = BaseDecoder()
    base.needs_records = True
    assert base.needs_records and pickle.loads(pickle.dumps(base)).needs_records
    assert not BaseDecoder.needs_records and not BaseDecoder().needs_records


def test_marginal_decoder_needs_records_cannot_be_false():
    dec = MarginalDecoder()
    with pytest.raises(ValueError, match="needs_records cannot be set to False"):
        dec.needs_records = False
    dec.needs_records = True
    assert dec.needs_records


def _base_decoder(needs_records):
    dec = BaseDecoder(FLIPS)  # predicts L0 = D0 on COIN_CIRCUIT
    dec.needs_records = needs_records
    return dec


@pytest.mark.parametrize("kind", list(_CALLABLE_SAMPLERS))
@pytest.mark.parametrize(
    "make",
    [_base_decoder, lambda nr: CallableDecoder(_dets_decoder, needs_records=nr)],
    ids=["base", "callable"],
)
@pytest.mark.parametrize("needs_records", [False, True])
def test_samplers_pass_records_iff_needs_records(kind, make, needs_records):
    dec = make(needs_records)
    compiled = _CALLABLE_SAMPLERS[kind](dec).compiled_sampler_for_task(
        sinter.Task(circuit=COIN_CIRCUIT, decoder=dec.name)
    )
    seen = []
    inner = compiled.compiled_dem_decoder.decode_shots_bit_packed

    def spy(**kwargs):
        seen.append(kwargs["records"])
        return inner(**kwargs)

    compiled.compiled_dem_decoder.decode_shots_bit_packed = spy
    stats = compiled.sample(suggested_shots=64)
    assert stats.shots >= 64 and stats.errors == 0
    assert seen
    if needs_records:
        assert all(r is not None and r.shape[1] == 1 for r in seen)
    else:
        assert all(r is None for r in seen)


@pytest.mark.parametrize("needs_records", [False, True])
def test_tableside_without_records_samples_only_detection_events(needs_records):
    sampler = TablesideSampler(
        op_handler=LeakageUint8(),
        batch_size=4,
        dem_decoder=CallableDecoder(_dets_decoder, needs_records=needs_records),
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=COIN_CIRCUIT, decoder="_dets_decoder")
    )
    assert compiled.sample(suggested_shots=64).errors == 0
    # Records are only sampled (compile_sampler + m2d) when fetched; otherwise
    # the detection events come from compile_detector_sampler.
    fetched = compiled.tab_simulator._final_measurement_records is not None
    assert fetched is needs_records
