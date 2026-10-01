import numpy as np
import pytest
import stim

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import (
    LeakageUint8Coset,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator


@pytest.mark.parametrize("use_cpp", [True, False])
@pytest.mark.parametrize("sync_rng", [False, True])
@pytest.mark.parametrize(
    "prep_ops,reset_op,meas_op,expected_meas",
    [
        ("X 0", "R 0", "M 0", [0]),
        ("X 0", "MR 0", "M 0", [1, 0]),
        ("H 0\nZ 0", "RX 0", "MX 0", [0]),
        ("H 0\nZ 0", "MRX 0", "MX 0", [1, 0]),
        ("H 0\nS 0\nZ 0", "RY 0", "MY 0", [0]),
        ("H 0\nS 0\nZ 0", "MRY 0", "MY 0", [1, 0]),
    ],
)
def test_coset_resets_when_m_ref_is_one(
    use_cpp: bool,
    sync_rng: bool,
    prep_ops: str,
    reset_op: str,
    meas_op: str,
    expected_meas: list[int],
):
    """Regression test for N1 & N6: CosetsideSimulator resets and measure-resets when m_ref == 1."""
    circuit = stim.Circuit(f"{prep_ops}\n{reset_op}\n{meas_op}")
    batch_size = 16
    handler = LeakageUint8Coset().compile_op_handler(
        circuit=circuit, batch_size=batch_size
    )
    sim = CosetsideSimulator(
        circuit,
        compiled_op_handler=handler,
        batch_size=batch_size,
        seed=123,
        use_cpp_kernels=use_cpp,
        sync_tableside_rng=sync_rng,
    )
    sim.run()
    records = sim.get_final_measurement_records()
    assert records.shape == (batch_size, len(expected_meas))
    for idx, exp_val in enumerate(expected_meas):
        assert np.all(records[:, idx] == bool(exp_val))


@pytest.mark.parametrize("use_cpp", [True, False])
def test_coset_resets_when_m_ref_is_one_with_active_mrqf(use_cpp: bool):
    """Verify resets when m_ref == 1 and shots have active MRQF states (d >= 1)."""
    circuit = stim.Circuit(
        """
        I[LEAKAGE_TRANSITION_1: (0.5, 0-->2)] 0
        H 1
        CZ 0 1
        X 0
        MR 0
        M 0
        """
    )
    batch_size = 64
    handler = LeakageUint8Coset().compile_op_handler(
        circuit=circuit, batch_size=batch_size
    )
    sim = CosetsideSimulator(
        circuit,
        compiled_op_handler=handler,
        batch_size=batch_size,
        seed=42,
        use_cpp_kernels=use_cpp,
    )
    sim.run()
    records = sim.get_final_measurement_records()
    assert np.all(records[:, 1] == False)


@pytest.mark.parametrize(
    "tagged_op",
    [
        "I[CONDITIONED_ON_SELF: 0 1] 0",
        "I[LEAKAGE_TRANSITION_1: (0.0, 1-->2)] 0",
        "I[LEAKAGE_TRANSITION_1: (0.001, 1-->2)] 0",
        "II[LEAKAGE_TRANSITION_2: (0.0, 0_0-->2_2)] 0 1",
        "MPAD[LEAKAGE_MEASUREMENT: (0.0, 0) (0.0, 1) (1.0, 2) : 0] 0",
    ],
)
def test_conditioning_on_0_or_1_collapses_superposition(tagged_op: str):
    """Regression test for Point 2 (Q2): conditioning on 0 or 1 collapses Z-basis superpositions."""
    circuit = stim.Circuit(
        f"""
        H 0
        {tagged_op}
        H 0
        M 0
        """
    )
    num_shots = 200

    # TablesideSimulator
    tab_handler = LeakageUint8Tableau().compile_op_handler(
        circuit=circuit, batch_size=1
    )
    tab_sim = TablesideSimulator(
        circuit, compiled_op_handler=tab_handler, seed=777
    )
    tab_outcomes = []
    for _ in range(num_shots):
        tab_sim.clear()
        tab_sim.run()
        tab_outcomes.append(bool(tab_sim.get_final_measurement_records()[0, -1]))
    tab_rate = np.mean(tab_outcomes)
    assert 0.3 < tab_rate < 0.7, (
        f"Expected ~0.5 after Z-collapse of |+> in Tableside for {tagged_op}, got {tab_rate}"
    )

    # CosetsideSimulator (both C++ and Python paths)
    for use_cpp in (True, False):
        cos_handler = LeakageUint8Coset().compile_op_handler(
            circuit=circuit, batch_size=num_shots
        )
        cos_sim = CosetsideSimulator(
            circuit,
            compiled_op_handler=cos_handler,
            batch_size=num_shots,
            seed=777,
            use_cpp_kernels=use_cpp,
        )
        cos_sim.run()
        cos_rate = float(np.mean(cos_sim.get_final_measurement_records()[:, -1]))
        assert 0.3 < cos_rate < 0.7, (
            f"Expected ~0.5 after Z-collapse of |+> in Cosetside (cpp={use_cpp}) for {tagged_op}, got {cos_rate}"
        )


def test_seed_none_is_random_each_run():
    """Regression test for Point 4 (A5): seed=None produces random outcomes across instances and .clear()."""
    circuit = stim.Circuit(
        """
        R 0
        X_ERROR(0.5) 0
        I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 0
        M[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.5, 2)] 0
        """
    )
    batch_size = 64

    # 2. CosetsideSimulator with sync_tableside_rng=True and False, seed=None
    for sync_rng in (False, True):
        c1 = CosetsideSimulator(
            circuit,
            batch_size=batch_size,
            compiled_op_handler=LeakageUint8Coset().compile_op_handler(
                circuit=circuit, batch_size=batch_size
            ),
            seed=None,
            sync_tableside_rng=sync_rng,
        )
        c1.run()
        r_c1_a = c1.get_final_measurement_records().copy()
        c1.clear()
        c1.run()
        r_c1_b = c1.get_final_measurement_records().copy()

        c2 = CosetsideSimulator(
            circuit,
            batch_size=batch_size,
            compiled_op_handler=LeakageUint8Coset().compile_op_handler(
                circuit=circuit, batch_size=batch_size
            ),
            seed=None,
            sync_tableside_rng=sync_rng,
        )
        c2.run()
        r_c2 = c2.get_final_measurement_records().copy()

        assert not np.array_equal(r_c1_a, r_c1_b)
        assert not np.array_equal(r_c1_a, r_c2)

    # 3. TablesideSimulator with seed=None across .clear()
    t1 = TablesideSimulator(
        circuit,
        batch_size=1,
        compiled_op_handler=LeakageUint8Tableau().compile_op_handler(
            circuit=circuit, batch_size=1
        ),
        seed=None,
    )
    bits_t1 = []
    for _ in range(40):
        t1.clear()
        t1.run()
        bits_t1.append(int(t1.get_final_measurement_records()[0, 0]))
    assert len(set(bits_t1)) == 2, (
        "Expected both 0 and 1 across TablesideSimulator.clear() with seed=None"
    )


def test_repeated_qubit_targets_processed_in_order():
    """Regression test for Point 5 (A4): instructions with repeated targets are processed in order."""
    # 1. CX 0 1 1 0 on |10> -> CX 0 1 gives |11> -> CX 1 0 gives |01>
    c_cx = stim.Circuit(
        """
        X 0
        CX 0 1 1 0
        H 0 0
        S 1 1
        M 0 1
        """
    )
    stim_sample = c_cx.compile_sampler().sample(1)[0].tolist()
    assert stim_sample == [False, True]

    for sim_cls, factory_cls in [
        (FlipsideSimulator, LeakageUint8Flip),
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
    ]:
        b_size = 1 if sim_cls is TablesideSimulator else 4
        handler = factory_cls().compile_op_handler(
            circuit=c_cx, batch_size=b_size
        )
        sim = sim_cls(
            c_cx, compiled_op_handler=handler, batch_size=b_size, seed=11
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        assert np.all(recs[:, 0] == False)
        assert np.all(recs[:, 1] == True)

    # 2. Sequential leakage transition on repeated target: I[LEAKAGE_TRANSITION_1: (1.0, U-->2) (1.0, 2-->3)] 0 0
    c_trans = stim.Circuit(
        """
        I[LEAKAGE_TRANSITION_1: (1.0, U-->2) (1.0, 2-->3)] 0 0
        M[LEAKAGE_PROJECTION_Z: (0.0, 0) (0.0, 1) (0.0, 2) (1.0, 3)] 0
        """
    )
    for sim_cls, factory_cls in [
        (FlipsideSimulator, LeakageUint8Flip),
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
    ]:
        b_size = 1 if sim_cls is TablesideSimulator else 4
        handler = factory_cls().compile_op_handler(
            circuit=c_trans, batch_size=b_size
        )
        sim = sim_cls(
            c_trans, compiled_op_handler=handler, batch_size=b_size, seed=11
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        assert np.all(recs[:, 0] == True), (
            f"Expected state 3 (meas=1) for {sim_cls.__name__}"
        )


def test_conditioning_01_with_repeated_targets():
    """Regression test for Bugs 1 & 2: 0/1 conditioning combined with repeated/overlapping targets."""
    # 1. X[CONDITIONED_ON_SELF: 0] 0 0 starting from |0>:
    # Group 0 sees |0> -> applies X -> state becomes |1>.
    # Group 1 sees |1> -> condition 0 is False -> skipped -> final state is |1>.
    c1 = stim.Circuit(
        """
        X[CONDITIONED_ON_SELF: 0] 0 0
        M 0
        """
    )
    for sim_cls, factory_cls in [
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
    ]:
        b_size = 1 if sim_cls is TablesideSimulator else 8
        handler = factory_cls().compile_op_handler(
            circuit=c1, batch_size=b_size
        )
        sim = sim_cls(
            c1, compiled_op_handler=handler, batch_size=b_size, seed=42
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        assert np.all(recs[:, 0] == True), (
            f"Expected |1> for X[CONDITIONED_ON_SELF: 0] 0 0 in {sim_cls.__name__}"
        )

    # 2. X 0; H[CONDITIONED_ON_SELF: 1] 0 0; MZ 0:
    # Group 0 sees |1> -> applies H -> state is |->.
    # Group 1 must evaluate against intermediate tableau (|->), collapsing Z_0 to 0 (50%) or 1 (50%).
    # If collapsed to 0, group 1 skips H and state remains |0>. If collapsed to 1, group 1 applies H -> |-> (MZ is 50/50).
    # Overall MZ 0 cannot be deterministically 1.
    c2 = stim.Circuit(
        """
        X 0
        H[CONDITIONED_ON_SELF: 1] 0 0
        M 0
        """
    )
    for use_cpp in (True, False):
        handler = LeakageUint8Coset().compile_op_handler(
            circuit=c2, batch_size=64
        )
        sim = CosetsideSimulator(
            c2,
            compiled_op_handler=handler,
            batch_size=64,
            seed=123,
            use_cpp_kernels=use_cpp,
        )
        sim.run()
        recs = sim.get_final_measurement_records()[:, 0]
        assert np.any(recs == False) and np.any(recs == True), (
            f"Expected non-deterministic outcome for H[CONDITIONED_ON_SELF: 1] 0 0 (use_cpp={use_cpp})"
        )

    # 3. CONDITIONED_ON_OTHER with overlapping targets across groups
    c3 = stim.Circuit(
        """
        X[CONDITIONED_ON_OTHER: 0 : 0 0] 0 1
        M 0 1
        """
    )
    for sim_cls, factory_cls in [
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
    ]:
        b_size = 1 if sim_cls is TablesideSimulator else 8
        handler = factory_cls().compile_op_handler(
            circuit=c3, batch_size=b_size
        )
        sim = sim_cls(
            c3, compiled_op_handler=handler, batch_size=b_size, seed=42
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        # Group 0 checks q0 (|0>) -> applies X 0 -> q0 becomes |1>.
        # Group 1 checks q0 (now |1>) -> skips X 1 -> q1 stays |0>.
        assert np.all(recs[:, 0] == True)
        assert np.all(recs[:, 1] == False)


def test_leakage_transition_2_single_leg_collapse():
    """Regression test for Bug 3: LEAKAGE_TRANSITION_2 only collapses the leg conditioned on 0 or 1."""
    circuit = stim.Circuit(
        """
        H 0 1
        II[LEAKAGE_TRANSITION_2: (0.0, 1_U-->2_U)] 0 1
        MX 1
        """
    )
    for sim_cls, factory_cls in [
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
    ]:
        b_size = 1 if sim_cls is TablesideSimulator else 32
        handler = factory_cls().compile_op_handler(
            circuit=circuit, batch_size=b_size
        )
        sim = sim_cls(
            circuit, compiled_op_handler=handler, batch_size=b_size, seed=77
        )
        for _ in range(16 if sim_cls is TablesideSimulator else 1):
            sim.clear()
            sim.run()
            recs = sim.get_final_measurement_records()
            assert np.all(recs[:, 0] == False), (
                f"Leg 1 conditioned only on U should remain |+> in {sim_cls.__name__}"
            )


def test_conditional_pauli_noise_duplicate_targets():
    """Regression test for Bug 4: conditional X_ERROR(1.0) 0 0 cancels out on unleaked shots."""
    circuit = stim.Circuit(
        """
        I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 1
        X_ERROR[CONDITIONED_ON_OTHER: U : 1 1](1.0) 0 0
        M 0
        """
    )
    handler = LeakageUint8Coset().compile_op_handler(
        circuit=circuit, batch_size=64
    )
    sim = CosetsideSimulator(
        circuit, compiled_op_handler=handler, batch_size=64, seed=99
    )
    sim.run()
    recs = sim.get_final_measurement_records()[:, 0]
    assert np.all(recs == False), (
        "X_ERROR(1.0) 0 0 should apply X^2 = I on unleaked shots"
    )

