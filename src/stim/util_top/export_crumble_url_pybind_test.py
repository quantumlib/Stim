import stim


def test_to_crumble_url_simple():
    c = stim.Circuit("""
        QUBIT_COORDS(2, 1) 0
        QUBIT_COORDS(2, 2) 1
        H 0
        TICK
        CX 0 1
        TICK
        C_XYZ 0
        S 1
        TICK
        MZZ 1 0
    """)
    assert c.to_crumble_url() == "https://algassert.com/crumble#circuit=Q(2,1)0;Q(2,2)1;H_0;TICK;CX_0_1;TICK;C_XYZ_0;S_1;TICK;MZZ_1_0"


def test_to_crumble_url_complex():
    c = stim.Circuit.generated('surface_code:rotated_memory_x', distance=3, rounds=2, after_clifford_depolarization=0.001)
    assert 'DEPOLARIZE1' in c.to_crumble_url()
