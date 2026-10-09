"""DemGenerator contract and MarginalLeakageDemGenerator(loss_oracle=True)."""

import pickle

import numpy as np
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators import (
    DemGenerator,
    MarginalDecoder,
    MarginalLeakageDemGenerator,
)
from stimside.dem_generators.dem_decoding import decode_with_generated_dems
from stimside.dem_generators.dem_generator_marginal import (
    _Builder,
    _match_shot_sources,
)
from stimside.dem_generators.dem_generator_marginal_test import (
    REP_CIRCUIT,
    _dem_dict,
    _make_surface_code_leakage_circuit,
    _records,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_flip import CompiledFlipsideSampler, FlipsideSampler
from stimside.sampler_tableau import TablesideSampler
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.leakage_events import LeakageEvent

# REP_CIRCUIT's sources: op 2 (LEAKAGE_TRANSITION_1 on 0 2) leaks qubit 0 or 2.
LEAK_Q0 = [LeakageEvent(2, 0, 0, 2), LeakageEvent(8, 0, 2, 0)]
LEAK_Q2 = [LeakageEvent(2, 2, 0, 2)]
LEAK_BOTH = [LeakageEvent(2, 0, 0, 2), LeakageEvent(2, 2, 0, 2)]
FLAG_Q0, FLAG_Q2 = 1, 2  # REP_CIRCUIT's MPAD flag records


def _xor_dicts(*dicts: dict) -> dict:
    out: dict = {}
    for d in dicts:
        for key, p in d.items():
            q = out.get(key, 0.0)
            out[key] = p + q - 2 * p * q
    return out


def _assert_dicts_close(actual: dict, expected: dict) -> None:
    keys = {k for k, p in actual.items() if p > 1e-15} | {
        k for k, p in expected.items() if p > 1e-15
    }
    for key in keys:
        assert actual.get(key, 0.0) == pytest.approx(expected.get(key, 0.0), abs=1e-12), key


@pytest.mark.parametrize("loss_oracle", [False, True])
def test_base_class_contract(loss_oracle):
    gen = MarginalLeakageDemGenerator(
        unconditional_condition_on_U=False,
        decompose_errors=True,
        reweight_only=True,
        loss_oracle=loss_oracle,
    )
    assert isinstance(gen, DemGenerator)
    assert gen.unconditional_condition_on_U is False
    assert gen.decompose_errors and gen.reweight_only
    assert gen.loss_oracle is loss_oracle
    assert gen.needs_leakage_events is loss_oracle
    weighted = MarginalLeakageDemGenerator(unconditional_condition_on_U=False)
    for dec in (False, True):
        assert gen.base_dem(REP_CIRCUIT, decompose_errors=dec) == weighted.base_dem(
            REP_CIRCUIT, decompose_errors=dec
        )
    records = _records(REP_CIRCUIT, shots=2)
    out = gen(
        REP_CIRCUIT,
        records,
        decompose_errors=False,
        reweight_only=False,
        leakage_events=[[], []],
    )
    assert len(out) == 2
    assert out[0] == gen.base_dem(REP_CIRCUIT, decompose_errors=False)
    with pytest.raises(TypeError):
        DemGenerator()  # type: ignore[abstract]


def test_weighted_mode_ignores_leakage_events():
    gen = MarginalLeakageDemGenerator()
    records = _records(REP_CIRCUIT, FLAG_Q0, shots=2)
    assert list(gen(REP_CIRCUIT, records)) == list(
        gen(REP_CIRCUIT, records, leakage_events=[LEAK_Q2, []])
    )


def test_oracle_requires_matching_leakage_events():
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    records = _records(REP_CIRCUIT, shots=2)
    with pytest.raises(ValueError, match="leakage_events"):
        gen(REP_CIRCUIT, records)
    with pytest.raises(ValueError, match="for 1 shots"):
        gen(REP_CIRCUIT, records, leakage_events=[[]])


@pytest.mark.parametrize("uc", [True, False])
def test_oracle_dem_is_base_plus_envelopes_of_exactly_the_leaked_sources(uc):
    # REP_CIRCUIT's flags each have exactly one candidate source, so the weighted
    # DEM of a flagged shot is the base plus that source's (whole) envelope.
    oracle = MarginalLeakageDemGenerator(uc, loss_oracle=True)
    weighted = MarginalLeakageDemGenerator(uc)
    events = [[], LEAK_Q0, LEAK_Q2, LEAK_BOTH, [LeakageEvent(2, 2, 0, 2)]]
    dems = oracle(REP_CIRCUIT, _records(REP_CIRCUIT, shots=5), leakage_events=events)
    flagged = weighted(
        REP_CIRCUIT,
        np.array(
            [
                _records(REP_CIRCUIT)[0],
                _records(REP_CIRCUIT, FLAG_Q0)[0],
                _records(REP_CIRCUIT, FLAG_Q2)[0],
                _records(REP_CIRCUIT, FLAG_Q0, FLAG_Q2)[0],
            ]
        ),
    )
    base = oracle.base_dem(REP_CIRCUIT)
    assert dems[0] == base
    for k in range(4):
        _assert_dicts_close(_dem_dict(dems[k]), _dem_dict(flagged[k]))
    base_d = _dem_dict(base)
    # base + (env_q0 xor env_q2) with the envelopes read off the one-source shots.
    extra_q0 = _dem_dict(oracle(REP_CIRCUIT, _records(REP_CIRCUIT)[0], reweight_only=True, leakage_events=LEAK_Q0))
    extra_q2 = _dem_dict(oracle(REP_CIRCUIT, _records(REP_CIRCUIT)[0], reweight_only=True, leakage_events=LEAK_Q2))
    _assert_dicts_close(_dem_dict(dems[3]), _xor_dicts(base_d, extra_q0, extra_q2))
    # Shots with the same source set share one DEM object.
    assert dems[2] is dems[4]
    assert dems[1] is not dems[2]


def test_oracle_reweight_modes_match_weighted_single_candidate_flags():
    events = [[], LEAK_Q0, LEAK_BOTH]
    flag_records = np.array(
        [
            _records(REP_CIRCUIT)[0],
            _records(REP_CIRCUIT, FLAG_Q0)[0],
            _records(REP_CIRCUIT, FLAG_Q0, FLAG_Q2)[0],
        ]
    )
    for dec, rew in [(False, True), (True, False), (True, True)]:
        oracle = MarginalLeakageDemGenerator(decompose_errors=dec, reweight_only=rew, loss_oracle=True)
        weighted = MarginalLeakageDemGenerator(decompose_errors=dec, reweight_only=rew)
        got = oracle(REP_CIRCUIT, _records(REP_CIRCUIT, shots=3), leakage_events=events)
        want = weighted(REP_CIRCUIT, flag_records)
        for g, w in zip(got, want):
            if isinstance(w, np.ndarray):
                np.testing.assert_allclose(g, w, rtol=1e-12, atol=1e-12)
            else:
                _assert_dicts_close(_dem_dict(g), _dem_dict(w))
        if rew and not dec:
            assert len(got[0]) == 0  # a shot without leakage gets an empty update
        if rew and dec:
            assert got[0].shape == (0, 3)


def test_regeneration_and_slicing_carry_the_leakage_events():
    oracle = MarginalLeakageDemGenerator(loss_oracle=True)
    events = [LEAK_Q2, [], LEAK_Q0, LEAK_BOTH]
    dems = oracle(REP_CIRCUIT, _records(REP_CIRCUIT, shots=4), leakage_events=events)
    assert dems.leakage_events == events
    assert dems[1:3].leakage_events == events[1:3]
    direct = MarginalLeakageDemGenerator(
        decompose_errors=True, reweight_only=True, loss_oracle=True
    )(REP_CIRCUIT, _records(REP_CIRCUIT, shots=4), leakage_events=events)
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    for regen in (
        decode_with_generated_dems(decoder, dems, None, decompose_errors=True, reweight_only=True),
        decode_with_generated_dems(decoder, list(dems), None, decompose_errors=True, reweight_only=True),
    ):
        for r, d in zip(regen, direct):
            np.testing.assert_allclose(r, d)
    single = oracle(REP_CIRCUIT, _records(REP_CIRCUIT)[0], leakage_events=LEAK_Q0)
    regen_single = decode_with_generated_dems(
        decoder, single, None, decompose_errors=True, reweight_only=True
    )
    np.testing.assert_allclose(regen_single, direct[2])


def _source_weights(circuit: stim.Circuit) -> list[list[float]]:
    builder = _Builder(circuit, True)
    builder._find_flags()
    builder._build_touch_lists()
    sources = builder._find_sources()
    res = builder._trace_flows_fast(sources, len(sources), source_mode=True)
    offsets, weights = res[3], res[5]
    return [list(weights[offsets[s] : offsets[s + 1]]) for s in range(len(sources))]


def test_hop_branch_mixture_weights():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2)] 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->U)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2) (0.25, 2_U-->U_3)] 0 2
        CX 0 1 2 1
        M 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        """
    )
    # Root forks at the first hop (still leaked w.p. 1) and at the second one
    # (still leaked w.p. 0.75 * 0.5); the branches do not fork again.
    (weights,) = _source_weights(circuit)
    alive = 0.75 * 0.5
    expected_branches = [0.25, alive * 0.25, alive * 0.25]
    assert sorted(weights[1:]) == pytest.approx(sorted(expected_branches))
    assert weights[0] == pytest.approx(1 - sum(expected_branches))


def test_mixture_weights_sum_to_at_most_one_on_a_surface_code():
    circuit = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    all_weights = _source_weights(circuit)
    assert len(all_weights) > 0
    for weights in all_weights:
        assert all(w >= 0 for w in weights)
        assert sum(weights) <= 1 + 1e-12
        assert weights[0] >= 0


def test_hop_targets_and_spreads_are_ignored_and_counted():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2) (0.1, 2_U-->2_2) (0.01, U_U-->U_2)] 0 2
        M 0 1 2
        """
    )
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    gen.base_dem(circuit)
    analysis = gen._analyses[False]
    hop = [LeakageEvent(1, 0, 0, 2), LeakageEvent(2, 0, 2, 0), LeakageEvent(2, 2, 0, 2)]
    spread = [LeakageEvent(1, 0, 0, 2), LeakageEvent(2, 2, 0, 2)]
    fresh = [LeakageEvent(2, 2, 0, 2)]
    src_q0 = analysis.source_lookup[(1, 0, 2)]
    src_fresh = analysis.source_lookup[(2, 2, 2)]
    assert _match_shot_sources(analysis, hop) == ([src_q0], 1)
    assert _match_shot_sources(analysis, spread) == ([src_q0], 1)
    assert _match_shot_sources(analysis, fresh) == ([src_fresh], 0)
    # Events are matched against the partner's state before the op, whatever their order.
    assert _match_shot_sources(analysis, list(reversed(hop))) == ([src_q0], 1)


def test_unmatched_event_and_repeated_qubits_raise():
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    records = _records(REP_CIRCUIT)[0]
    with pytest.raises(ValueError, match="matches no leakage source"):
        gen(REP_CIRCUIT, records, leakage_events=[LeakageEvent(3, 0, 0, 2)])
    with pytest.raises(ValueError, match="matches no leakage source"):
        gen(REP_CIRCUIT, records, leakage_events=[LeakageEvent(2, 0, 0, 3)])
    repeated = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1
        II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)] 0 1 1 2
        M 0 1 2
        """
    )
    with pytest.raises(NotImplementedError, match="listed twice"):
        gen.base_dem(repeated)


def test_events_of_simulated_repeat_and_shift_coords_circuit_match_sources():
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        REPEAT 3 {
            SHIFT_COORDS(0, 0, 1)
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CX 0 1
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
            MR 1
            DETECTOR(0, 0, 0) rec[-1]
        }
        """
    )
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    handler = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=1)
    sim = TablesideSimulator(
        circuit=circuit, compiled_op_handler=handler, seed=3, record_leakage_events=True
    )
    sim.run()
    (events,) = sim.get_leakage_events()
    assert [ev.op_index for ev in events] == [4, 7, 11, 14, 18, 21]
    analysis_dem = gen(circuit, sim.get_final_measurement_records()[0], leakage_events=events)
    matched, ignored = _match_shot_sources(gen._analyses[False], events)
    assert len(matched) == 3 and ignored == 0
    assert analysis_dem != gen.base_dem(circuit)


def _leaky_circuit() -> stim.Circuit:
    return _make_surface_code_leakage_circuit(distance=3, rounds=2)


@pytest.mark.parametrize("batch_size", [1, 16])
def test_tableside_sampler_runs_the_oracle(batch_size):
    sampler = TablesideSampler(
        op_handler=LeakageUint8(),
        batch_size=batch_size,
        dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator(loss_oracle=True)),
        seed=5,
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=_leaky_circuit(), decoder="pymatching")
    )
    assert compiled.tab_simulator.record_leakage_events
    stats = compiled.sample(32)
    assert stats.shots >= 32


def test_cosetside_sampler_runs_the_oracle():
    sampler = CosetsideSampler(
        op_handler=LeakageUint8Coset(),
        batch_size=16,
        dem_decoder=MarginalDecoder(
            MarginalLeakageDemGenerator(loss_oracle=True, decompose_errors=True)
        ),
        seed=5,
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=_leaky_circuit(), decoder="pymatching")
    )
    assert compiled.simulator.record_leakage_events
    stats = compiled.sample(32)
    assert stats.shots >= 32


def test_weighted_samplers_do_not_record_events():
    sampler = TablesideSampler(
        op_handler=LeakageUint8(),
        batch_size=2,
        dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator()),
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=REP_CIRCUIT, decoder="pymatching")
    )
    assert not compiled.tab_simulator.record_leakage_events


def test_flipside_rejects_the_oracle():
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    with pytest.raises(ValueError, match="leakage events"):
        FlipsideSampler(op_handler=LeakageUint8Flip(), dem_decoder=MarginalDecoder(gen))
    with pytest.raises(ValueError, match="leakage events"):
        CompiledFlipsideSampler(
            circuit=REP_CIRCUIT,
            compiled_op_handler=LeakageUint8Flip().compile_op_handler(
                circuit=REP_CIRCUIT, batch_size=8
            ),
            batch_size=8,
            compiled_dem_decoder=MarginalDecoder(gen).compile_for_task(
                sinter.Task(circuit=REP_CIRCUIT)
            ),
            needs_records=True,
            needs_leakage_events=True,
        )


def test_oracle_generator_and_sampler_pickle():
    gen = MarginalLeakageDemGenerator(loss_oracle=True)
    first = gen(REP_CIRCUIT, _records(REP_CIRCUIT)[0], leakage_events=LEAK_Q0)
    clone = pickle.loads(pickle.dumps(gen))
    assert clone.loss_oracle and clone.needs_leakage_events
    assert clone(REP_CIRCUIT, _records(REP_CIRCUIT)[0], leakage_events=LEAK_Q0) == first
    sampler = pickle.loads(
        pickle.dumps(TablesideSampler(op_handler=LeakageUint8(), dem_decoder=MarginalDecoder(gen)))
    )
    assert sampler.dem_decoder.needs_leakage_events


def test_sinter_collect_with_two_workers_in_oracle_mode():
    stats = sinter.collect(
        num_workers=2,
        tasks=[sinter.Task(circuit=_leaky_circuit(), decoder="pymatching")],
        custom_decoders={
            "pymatching": TablesideSampler(
                op_handler=LeakageUint8(),
                batch_size=8,
                dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator(loss_oracle=True)),
                seed=11,
            )
        },
        max_shots=64,
    )
    assert sum(s.shots for s in stats) >= 64
