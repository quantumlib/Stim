import stimflow


def test_to_svg():
    patch = stimflow.Patch(
        [stimflow.Tile(bases="X", data_qubits=[0, 1, 1j, 1 + 1j], measure_qubit=0.5 + 0.5j)]
    )
    assert (
        patch.to_svg()
        == """
<svg viewBox="0 0 500 500" xmlns="http://www.w3.org/2000/svg">
<path d="M250.0,416.66666666666663 L 250.0,250.0 L 416.66666666666663,250.0 L 416.66666666666663,416.66666666666663 L 250.0,416.66666666666663" fill="#FF8080" stroke="none" />
<text x="250.0" y="83.33333333333333" fill="black" font-size="83.33333333333331" text-anchor="middle" dominant-baseline="central">0</text>
<text x="416.66666666666663" y="83.33333333333333" fill="black" font-size="83.33333333333331" text-anchor="middle" dominant-baseline="central">1</text>
<text x="83.33333333333333" y="250.0" fill="black" font-size="83.33333333333331" text-anchor="middle" dominant-baseline="central">0i</text>
<text x="83.33333333333333" y="416.66666666666663" fill="black" font-size="83.33333333333331" text-anchor="middle" dominant-baseline="central">1i</text>
<path d="M250.0,416.66666666666663 L 250.0,250.0 L 416.66666666666663,250.0 L 416.66666666666663,416.66666666666663 L 250.0,416.66666666666663" fill="none" stroke="black" stroke-width="3.3333333333333326"  />
<circle cx="250.0" cy="250.0" r="16.666666666666664" fill="black" stroke="none" />
<circle cx="416.66666666666663" cy="250.0" r="16.666666666666664" fill="black" stroke="none" />
<circle cx="250.0" cy="416.66666666666663" r="16.666666666666664" fill="black" stroke="none" />
<circle cx="416.66666666666663" cy="416.66666666666663" r="16.666666666666664" fill="black" stroke="none" />
<rect fill="none" stroke="#999" stroke-width="1.6666666666666665" x="0.0" y="0.0" width="500.0" height="500.0" />
</svg>
    """.strip()  # noqa: E501
    )


def test_with_remaining_degrees_of_freedom_as_logicals():
    patch = stimflow.Patch([stimflow.PauliMap({"X": [0, 1, 2, 3]}), stimflow.PauliMap({"Z": [0, 1, 2, 3]})])
    code = patch.with_remaining_degrees_of_freedom_as_logicals()
    assert code.stabilizers == patch
    assert len(code.logicals) == 2
    code.verify()
    assert code == stimflow.StabilizerCode(
        stabilizers=[stimflow.PauliMap({"X": [0, 1, 2, 3]}), stimflow.PauliMap({"Z": [0, 1, 2, 3]})],
        logicals=[
            # Not sure how stable the exact answer is.
            (stimflow.PauliMap({"X": [1, 2]}, obs_name="X1"), stimflow.PauliMap({"Z": [0, 2]}, obs_name="Z1")),
            (stimflow.PauliMap({"X": [1, 3]}, obs_name="X2"), stimflow.PauliMap({"Z": [0, 3]}, obs_name="Z2")),
        ],
    )


def test_partitioned_tiles():
    t0 = stimflow.Tile(data_qubits=[0, 1, 2, 3], bases="X", flags={"A"})
    t1 = stimflow.Tile(data_qubits=[2, 3, 4, 5], bases="Z", flags={"B"})
    t2 = stimflow.Tile(data_qubits=[4, 5, 6, 7], bases="Y", flags={"C"})
    patch = stimflow.Patch([t0, t1, t2])
    assert patch.partitioned_tiles == ((t0, t2), (t1,))
