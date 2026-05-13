import stimflow


def test_from_auto_rewrite_xs():
    result = stimflow.ChunkReflow.from_auto_rewrite(
        inputs=[
            stimflow.PauliMap({"X": [2, 3]}),
            stimflow.PauliMap({"X": [3, 4]}),
            stimflow.PauliMap({"X": [4, 5, 6]}),
            stimflow.PauliMap({"X": [5, 7]}),
            stimflow.PauliMap({"X": [8, 6]}),
            stimflow.PauliMap({"X": [7, 6]}),
        ],
        out2in={
            stimflow.PauliMap({"X": [2, 3]}): [stimflow.PauliMap({"X": [2, 3]})],
            stimflow.PauliMap({"X": [2]}): "auto",
        },
    )
    assert result == stimflow.ChunkReflow(
        out2in={
            stimflow.PauliMap({"X": [2, 3]}): [stimflow.PauliMap({"X": [2, 3]})],
            stimflow.PauliMap({"X": [2]}): [
                stimflow.PauliMap({"X": [2, 3]}),
                stimflow.PauliMap({"X": [3, 4]}),
                stimflow.PauliMap({"X": [4, 5, 6]}),
                stimflow.PauliMap({"X": [5, 7]}),
                stimflow.PauliMap({"X": [7, 6]}),
            ],
        }
    )


def test_from_auto_rewrite_xyz():
    result = stimflow.ChunkReflow.from_auto_rewrite(
        inputs=[stimflow.PauliMap({"X": [2, 3]}), stimflow.PauliMap({"Z": [2, 3]})],
        out2in={stimflow.PauliMap({"Y": [2, 3]}): "auto"},
    )
    assert result == stimflow.ChunkReflow(
        out2in={
            stimflow.PauliMap({"Y": [2, 3]}): [stimflow.PauliMap({"X": [2, 3]}), stimflow.PauliMap({"Z": [2, 3]})]
        }
    )


def test_from_auto_rewrite_keyed():
    result = stimflow.ChunkReflow.from_auto_rewrite(
        inputs=[stimflow.PauliMap({"X": [2, 3]}), stimflow.PauliMap({"Z": [2, 3]}).with_obs_name("test")],
        out2in={stimflow.PauliMap({"Y": [2, 3]}): "auto"},
    )
    assert result == stimflow.ChunkReflow(
        out2in={
            stimflow.PauliMap({"Y": [2, 3]}): [
                stimflow.PauliMap({"X": [2, 3]}),
                stimflow.PauliMap({"Z": [2, 3]}).with_obs_name("test"),
            ]
        }
    )


def test_from_auto_rewrite_transitions_using_stable():
    x12 = stimflow.PauliMap.from_xs([1, 2])
    y12 = stimflow.PauliMap.from_ys([1, 2])
    z12 = stimflow.PauliMap.from_zs([1, 2])
    x1 = stimflow.PauliMap.from_xs([1])
    x2 = stimflow.PauliMap.from_xs([2])
    assert stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable(
        stable=[x12], transitions=[(x1, x2)]
    ) == stimflow.ChunkReflow(out2in={x12: [x12], x2: [x12, x1]})
    assert stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable(
        stable=[y12], transitions=[(z12.with_obs_name("test"), x12.with_obs_name("test"))]
    ) == stimflow.ChunkReflow(out2in={y12: [y12], x12.with_obs_name("test"): [y12, z12.with_obs_name("test")]})
