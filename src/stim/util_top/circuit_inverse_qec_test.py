import stim


def test_inv_circuit():
    inv_circuit, inv_flows = stim.Circuit().time_reversed_for_flows([])
    assert inv_circuit == stim.Circuit()
    assert inv_flows == []

    inv_circuit, inv_flows = stim.Circuit("""
        R 0
        H 0
        MX 0
        DETECTOR rec[-1]
    """).time_reversed_for_flows([])
    assert inv_circuit == stim.Circuit("""
        RX 0
        H 0
        M 0
        DETECTOR rec[-1]
    """)
    assert inv_flows == []

    inv_circuit, inv_flows = stim.Circuit("""
        M 0
    """).time_reversed_for_flows([stim.Flow('Z -> rec[-1]')])
    assert inv_circuit == stim.Circuit("""
        R 0
    """)
    assert inv_flows == [stim.Flow('1 -> Z')]
    assert inv_circuit.has_all_flows(inv_flows, unsigned=True)

    inv_circuit, inv_flows = stim.Circuit("""
        R 0
    """).time_reversed_for_flows([stim.Flow('1 -> Z')])
    assert inv_circuit == stim.Circuit("""
        M 0
    """)
    assert inv_flows == [stim.Flow('Z -> rec[-1]')]

    inv_circuit, inv_flows = stim.Circuit("""
        M 0
    """).time_reversed_for_flows([stim.Flow('1 -> Z xor rec[-1]')])
    assert inv_circuit == stim.Circuit("""
        M 0
    """)
    assert inv_flows == [stim.Flow('Z -> rec[-1]')]

    inv_circuit, inv_flows = stim.Circuit("""
        M 0
    """).time_reversed_for_flows(
        flows=[stim.Flow('Z -> rec[-1]')],
        dont_turn_measurements_into_resets=True,
    )
    assert inv_circuit == stim.Circuit("""
        M 0
    """)
    assert inv_flows == [stim.Flow('1 -> Z xor rec[-1]')]

    inv_circuit, inv_flows = stim.Circuit("""
        MR(0.125) 0
    """).time_reversed_for_flows([])
    assert inv_circuit == stim.Circuit("""
        MR 0
        X_ERROR(0.125) 0
    """)
    assert inv_flows == []


def test_inv_circuit_surface_code():
    circuit = stim.Circuit.generated(
        "surface_code:rotated_memory_x",
        distance=3,
        rounds=2,
    )
    det_regions = circuit.detecting_regions()
    inv_circuit, _ = circuit.time_reversed_for_flows([])
    keys = det_regions.keys()
    inv_det_regions = inv_circuit.detecting_regions()
    assert inv_det_regions.keys() == keys

    # Check observable is time reversed.
    num_ticks = circuit.num_ticks
    l0 = stim.target_logical_observable_id(0)
    assert inv_circuit.num_ticks == num_ticks
    assert {num_ticks - t - 1: v for t, v in det_regions[l0].items()} == inv_det_regions[l0]

    # Check all regions are time reversed (though may be indexed differently)
    original_region_sets_reversed = set()
    for k, v in det_regions.items():
        original_region_sets_reversed.add(frozenset({num_ticks - t - 1: str(p) for t, p in v.items()}.items()))
    inv_region_sets = set()
    for k, v in inv_det_regions.items():
        inv_region_sets.add(frozenset({t: str(p) for t, p in v.items()}.items()))
    assert len(original_region_sets_reversed) == len(det_regions)
    assert original_region_sets_reversed == inv_region_sets


def test_more_flow_qubits_than_circuit_qubits():
    flows = [
        stim.Flow("X300 -> X300"),
        stim.Flow("X2*Z301 -> Z2*Z301"),
    ]
    circuit = stim.Circuit("H 2")
    assert circuit.has_flow(flows[0])
    assert circuit.has_flow(flows[1])
    assert circuit.has_all_flows(flows)
    new_circuit, new_flows = circuit.time_reversed_for_flows(flows=flows)
    assert new_circuit == circuit
    assert new_flows == [
        stim.Flow("X300 -> X300"),
        stim.Flow("Z2*Z301 -> X2*Z301"),
    ]


def test_measurement_ordering():
    circuit = stim.Circuit("""
        M 0 1
    """)
    flows = [
        stim.Flow("I -> Z0 xor rec[-2]"),
        stim.Flow("I -> Z1 xor rec[-1]"),
    ]
    assert circuit.has_all_flows(flows, unsigned=True)
    new_circuit, new_flows = circuit.time_reversed_for_flows(flows)
    assert new_circuit.has_all_flows(new_flows, unsigned=True)


def test_measurement_ordering_2():
    circuit = stim.Circuit("""
        MZZ 0 1 2 3
    """)
    flows = [
        stim.Flow("I -> Z0*Z1 xor rec[-2]"),
        stim.Flow("I -> Z2*Z3 xor rec[-1]"),
    ]
    assert circuit.has_all_flows(flows, unsigned=True)
    new_circuit, new_flows = circuit.time_reversed_for_flows(flows)
    assert new_circuit.has_all_flows(new_flows, unsigned=True)


def test_measurement_ordering_3():
    circuit = stim.Circuit("""
        MR 0 1
    """)
    flows = [
        stim.Flow("Z0 -> rec[-2]"),
        stim.Flow("Z1 -> rec[-1]"),
        stim.Flow("I -> Z0"),
        stim.Flow("I -> Z1"),
    ]
    assert circuit.has_all_flows(flows, unsigned=True)
    new_circuit, new_flows = circuit.time_reversed_for_flows(flows)
    assert new_circuit.has_all_flows(new_flows, unsigned=True)
