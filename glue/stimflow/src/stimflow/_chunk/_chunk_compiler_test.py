import stim

import stimflow


def test_chunk_compiler_q2i():
    compiler = stimflow.ChunkCompiler()
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(2, 3) 0
            H 0
        """
            ),
            flows=[],
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
                QUBIT_COORDS(2, 4) 0
                QUBIT_COORDS(2, 3) 1
                CX 0 1
                """
            ),
            flows=[],
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
                QUBIT_COORDS(2, 4) 0
                S 0
                """
            ),
            flows=[],
        )
    )
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(2, 3) 0
        QUBIT_COORDS(2, 4) 1
        H 0
        TICK
        CX 1 0
        TICK
        S 1
            """
    )


def test_chunk_compiler_single_flow():
    compiler = stimflow.ChunkCompiler()
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
                QUBIT_COORDS(1, 2) 0
                R 0
                """
            ),
            flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([1 + 2j]), center=3 + 5j)],
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
                QUBIT_COORDS(1, 2) 0
                M 0
                """
            ),
            flows=[stimflow.Flow(start=stimflow.PauliMap.from_zs([1 + 2j]), measurement_indices=[0], center=3 + 5j)],
        )
    )
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(1, 2) 0
        R 0
        TICK
        M 0
        DETECTOR(3, 5, 0) rec[-1]
            """
    )


def test_chunk_compiler_obs_flow_eager_dump():
    compiler = stimflow.ChunkCompiler()
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            R 0
        """
            ),
            flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([0]).with_name(0), center=0)],
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            MR 0
        """
            ),
            flows=[
                stimflow.Flow(
                    start=stimflow.PauliMap.from_zs([0]).with_name(0),
                    end=stimflow.PauliMap.from_zs([0]).with_name(0),
                    measurement_indices=[0],
                    center=0,
                )
            ],
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            M 0
        """
            ),
            flows=[stimflow.Flow(start=stimflow.PauliMap.from_zs([0]).with_name(0), measurement_indices=[0], center=0)],
        )
    )
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        MR 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        TICK
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )


def test_chunk_compiler_loop():
    compiler = stimflow.ChunkCompiler()
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            QUBIT_COORDS(0, 1) 1
            QUBIT_COORDS(0, 2) 2
            QUBIT_COORDS(0, 3) 3
            R 0 1 2 3
        """
            ),
            flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([k]), center=0) for k in range(4)],
        )
    )
    compiler.append(
        stimflow.ChunkLoop(
            [
                stimflow.Chunk(
                    circuit=stim.Circuit(
                        """
                    QUBIT_COORDS(0, 0) 0
                    QUBIT_COORDS(0, 1) 1
                    QUBIT_COORDS(0, 2) 2
                    QUBIT_COORDS(0, 3) 3
                    SWAP 0 1
                    SWAP 1 2
                    SWAP 2 3
                    M 3
                """
                    ),
                    flows=[
                        stimflow.Flow(start=stimflow.PauliMap.from_zs([0]), measurement_indices=[0], center=0),
                        stimflow.Flow(end=stimflow.PauliMap.from_zs([3]), measurement_indices=[0], center=0),
                        stimflow.Flow(
                            start=stimflow.PauliMap.from_zs([1]), end=stimflow.PauliMap.from_zs([0]), center=0
                        ),
                        stimflow.Flow(
                            start=stimflow.PauliMap.from_zs([2]), end=stimflow.PauliMap.from_zs([1]), center=0
                        ),
                        stimflow.Flow(
                            start=stimflow.PauliMap.from_zs([3]), end=stimflow.PauliMap.from_zs([2]), center=0
                        ),
                    ],
                )
            ],
            repetitions=1000,
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            QUBIT_COORDS(0, 1) 1
            QUBIT_COORDS(0, 2) 2
            QUBIT_COORDS(0, 3) 3
            M 0 1 2 3
        """
            ),
            flows=[stimflow.Flow(start=stimflow.PauliMap.from_zs([k]), measurement_indices=[k], center=0) for k in range(4)],
        )
    )
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(0, 2) 2
        QUBIT_COORDS(0, 3) 3
        R 0 1 2 3
        TICK
        REPEAT 4 {
            SWAP 0 1 1 2 2 3
            M 3
            DETECTOR(0, 0, 0) rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
        }
        REPEAT 996 {
            SWAP 0 1 1 2 2 3
            M 3
            DETECTOR(0, 0, 0) rec[-5] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
        }
        M 0 1 2 3
        DETECTOR(0, 0, 0) rec[-8] rec[-4]
        DETECTOR(0, 0, 1) rec[-7] rec[-3]
        DETECTOR(0, 0, 2) rec[-6] rec[-2]
        DETECTOR(0, 0, 3) rec[-5] rec[-1]
        """
    )


def test_chunk_compiler_loop_obs():
    compiler = stimflow.ChunkCompiler()
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            R 0
        """
            ),
            flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([0]).with_name(3), center=0)],
        )
    )
    compiler.append(
        stimflow.ChunkLoop(
            [
                stimflow.Chunk(
                    circuit=stim.Circuit(
                        """
                    QUBIT_COORDS(0, 0) 0
                    MR 0
                """
                    ),
                    flows=[
                        stimflow.Flow(
                            start=stimflow.PauliMap.from_zs([0]).with_name(3),
                            end=stimflow.PauliMap.from_zs([0]).with_name(3),
                            measurement_indices=[0],
                            center=0,
                        )
                    ],
                )
            ],
            repetitions=1000,
        )
    )
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 0) 0
            M 0
        """
            ),
            flows=[stimflow.Flow(start=stimflow.PauliMap.from_zs([0]).with_name(3), measurement_indices=[0], center=0)],
        )
    )
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        REPEAT 1000 {
            MR 0
            OBSERVABLE_INCLUDE(0) rec[-1]
            TICK
        }
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )


def test_compile_postselected_chunks():
    chunk1 = stimflow.Chunk(
        circuit=stim.Circuit(
            """
            R 0
        """
        ),
        q2i={0: 0},
        flows=[stimflow.Flow(center=0, end=stimflow.PauliMap({0: "Z"}))],
    )
    chunk2 = stimflow.Chunk(
        circuit=stim.Circuit(
            """
            M 0
        """
        ),
        q2i={0: 0},
        flows=[
            stimflow.Flow(center=0, end=stimflow.PauliMap({0: "Z"}), measurement_indices=[0]),
            stimflow.Flow(center=0, start=stimflow.PauliMap({0: "Z"}), measurement_indices=[0]),
        ],
    )
    chunk3 = stimflow.Chunk(
        circuit=stim.Circuit(
            """
            MR 0
        """
        ),
        q2i={0: 0},
        flows=[stimflow.Flow(center=0, start=stimflow.PauliMap({0: "Z"}), measurement_indices=[0])],
    )

    compiler = stimflow.ChunkCompiler()
    compiler.append(chunk1)
    compiler.append(chunk2)
    compiler.append(chunk3)
    assert compiler.finish_circuit().flattened() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        M 0
        DETECTOR(0, 0, 0) rec[-1]
        TICK
        MR 0
        DETECTOR(0, 0, 1) rec[-2] rec[-1]
        """
    )

    compiler = stimflow.ChunkCompiler(metadata_func=lambda flow: stimflow.FlowMetadata(
        extra_coords=[999] if "postselect" in flow.flags else []
    ))
    compiler.append(chunk1.with_edits(flows=[f.with_edits(flags={"postselect"}) for f in chunk1.flows]))
    compiler.append(chunk2)
    compiler.append(chunk3)
    assert compiler.finish_circuit().flattened() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
            R 0
            TICK
            M 0
            DETECTOR(0, 0, 0, 999) rec[-1]
            TICK
            MR 0
            DETECTOR(0, 0, 1) rec[-2] rec[-1]
            """
    )

    compiler = stimflow.ChunkCompiler(metadata_func=lambda flow: stimflow.FlowMetadata(
        extra_coords=[999] if "postselect" in flow.flags else []
    ))
    compiler.append(chunk1)
    compiler.append(chunk2.with_edits(flows=[f.with_edits(flags={"postselect"}) for f in chunk2.flows]))
    compiler.append(chunk3)
    assert compiler.finish_circuit().flattened() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        M 0
        DETECTOR(0, 0, 0, 999) rec[-1]
        TICK
        MR 0
        DETECTOR(0, 0, 1, 999) rec[-2] rec[-1]
    """
    )

    compiler = stimflow.ChunkCompiler(metadata_func=lambda flow: stimflow.FlowMetadata(
        extra_coords=[999] if "postselect" in flow.flags else []
    ))
    compiler.append(chunk1)
    compiler.append(chunk2)
    compiler.append(chunk3.with_edits(flows=[f.with_edits(flags={"postselect"}) for f in chunk3.flows]))
    assert compiler.finish_circuit().flattened() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        M 0
        DETECTOR(0, 0, 0) rec[-1]
        TICK
        MR 0
        DETECTOR(0, 0, 1, 999) rec[-2] rec[-1]
    """
    )

    compiler = stimflow.ChunkCompiler(metadata_func=lambda flow: stimflow.FlowMetadata(
        extra_coords=[999] if "postselect" in flow.flags else []
    ))
    compiler.append(chunk1)
    compiler.append(chunk2.with_edits(
        flows=[f.with_edits(flags={"postselect"}) if f.start else f for f in chunk2.flows]
    ))
    compiler.append(chunk3)
    assert compiler.finish_circuit().flattened() == stim.Circuit(
        """
            QUBIT_COORDS(0, 0) 0
            R 0
            TICK
            M 0
            DETECTOR(0, 0, 0, 999) rec[-1]
            TICK
            MR 0
            DETECTOR(0, 0, 1) rec[-2] rec[-1]
        """
    )


def test_chunk_compiler_propagate_discards():
    c = stimflow.ChunkCompiler()
    xx = stimflow.PauliMap.from_xs([0, 1])
    zz = stimflow.PauliMap.from_zs([0, 1])
    c.append(
        stimflow.Chunk(
            stim.Circuit(
                """
                    R 0 1
                """
            ),
            q2i={0: 0, 1: 1},
            flows=[stimflow.Flow(end=zz, center=0)],
            discarded_outputs=[xx],
        )
    )
    c.append(
        stimflow.Chunk(
            stim.Circuit(
                """
                    MZZ 0 1
                """
            ),
            q2i={0: 0, 1: 1},
            flows=[
                stimflow.Flow(start=zz, center=0, measurement_indices=[0]),
                stimflow.Flow(end=zz, center=0, measurement_indices=[0]),
                stimflow.Flow(start=xx, end=xx, center=0),
            ],
        )
    )
    c.append(
        stimflow.Chunk(
            stim.Circuit(
                """
                    MX 0 1
                """
            ),
            q2i={0: 0, 1: 1},
            discarded_inputs=[zz],
            flows=[stimflow.Flow(start=xx, center=0, measurement_indices=[0])],
        )
    )
    assert c.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        TICK
        MZZ 0 1
        DETECTOR(0, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        MX 0 1
    """
    )


def test_drop_observable_later():
    c = stimflow.ChunkCompiler()
    xx = stimflow.PauliMap.from_xs([0, 1])
    zz = stimflow.PauliMap.from_zs([0, 1])
    c.append(
        stimflow.Chunk(
            stim.Circuit(
                """
            MPP X0*X1
            MPP Z0*Z1
        """
            ),
            q2i={0: 0, 1: 1},
            flows=[
                stimflow.Flow(end=zz.with_name("a"), measurement_indices=[1]),
                stimflow.Flow(end=xx.with_name("b"), measurement_indices=[0]),
            ],
        )
    )

    c.append(
        stimflow.Chunk(
            stim.Circuit(
                """
            MPP X0*X1
        """
            ),
            q2i={0: 0, 1: 1},
            discarded_inputs=[zz.with_name("a")],
            flows=[stimflow.Flow(start=xx.with_name("b"), measurement_indices=[0])],
        )
    )

    assert c.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        MPP X0*X1 Z0*Z1
        OBSERVABLE_INCLUDE(0) rec[-2]
        TICK
        MPP X0*X1
        OBSERVABLE_INCLUDE(0) rec[-1]
    """
    )


def test_chunk_negative_index_flow_measurement():
    chunk = stimflow.Chunk(
        circuit=stim.Circuit(
            """
            QUBIT_COORDS(0, 0) 0
            QUBIT_COORDS(1, 0) 1
            QUBIT_COORDS(2, 0) 2
            R 0 1 2
            M 0 1 2
        """
        ),
        flows=[
            stimflow.Flow(measurement_indices=[-1], center=0),
            stimflow.Flow(measurement_indices=[-2], center=0),
            stimflow.Flow(measurement_indices=[-3], center=0),
        ],
    )
    compiler = stimflow.ChunkCompiler()
    compiler.append(chunk)
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        R 0 1 2
        M 0 1 2
        DETECTOR(0, 0, 0) rec[-1]
        DETECTOR(0, 0, 1) rec[-2]
        DETECTOR(0, 0, 2) rec[-3]
    """
    )


def test_merge_ticks():
    q2i = {k: k for k in range(3)}
    init_chunk = stimflow.Chunk(
        q2i=q2i,
        circuit=stim.Circuit(
            """
            R 0 2
            TICK
            """
        ),
        flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([0, 2]))],
        wants_to_merge_with_next=True,
    )
    rep_chunk = stimflow.Chunk(
        q2i=q2i,
        circuit=stim.Circuit(
            """
            R 1
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            M 1
            """
        ),
        flows=[
            stimflow.Flow(start=stimflow.PauliMap.from_zs([0, 2]), measurement_indices=[0]),
            stimflow.Flow(end=stimflow.PauliMap.from_zs([0, 2]), measurement_indices=[-1]),
        ],
    )
    measure_chunk = init_chunk.time_reversed()

    compiler = stimflow.ChunkCompiler()
    compiler.append(init_chunk)
    compiler.append(rep_chunk)
    compiler.append(rep_chunk)
    compiler.append(measure_chunk)
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        R 0 2 1
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        M 1
        DETECTOR(1, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        R 1
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        M 1
        DETECTOR(1, 0, 0) rec[-2] rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        M 2 0
        DETECTOR(1, 0, 0) rec[-3] rec[-2] rec[-1]
    """
    )


def test_preserves_tags():
    compiler = stimflow.ChunkCompiler()
    builder = stimflow.ChunkBuilder()
    builder.append("R", [0])
    builder.append("TICK")
    builder.append("X", [0], tag="test")
    builder.append("TICK")
    builder.add_flow(end=stimflow.PauliMap({0: "Z"}), center=2)
    compiler.append(builder.finish_chunk())
    compiler.append_magic_end_chunk()
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        TICK
        X[test] 0
        TICK
        MPP Z0
        DETECTOR(1, 0, 0) rec[-1]
    """
    )


def test_merges_with_loop():
    compiler = stimflow.ChunkCompiler()
    s = stimflow.Chunk(
        circuit=stim.Circuit(
            """
            QUBIT_COORDS(0, 0) 0
            R 0
        """
        ),
        flows=[stimflow.Flow(end=stimflow.PauliMap.from_zs([0]))],
        wants_to_merge_with_next=True,
    )
    compiler.append(s)
    compiler.append(
        stimflow.Chunk(
            circuit=stim.Circuit(
                """
            QUBIT_COORDS(0, 1) 0
            R 0
            M 0
        """
            ),
            flows=[
                stimflow.Flow(start=stimflow.PauliMap.from_zs([0]), end=stimflow.PauliMap.from_zs([0])),
                stimflow.Flow(measurement_indices=[0]),
            ],
        )
        * 5
    )
    compiler.append(s.time_reversed())
    assert compiler.finish_circuit() == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        R 0 1
        M 1
        DETECTOR(-1, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        REPEAT 3 {
            R 1
            M 1
            DETECTOR(-1, 0, 0) rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
        }
        R 1
        M 1
        DETECTOR(-1, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        M 0
        DETECTOR(0, 0, 0) rec[-1]
    """
    )
