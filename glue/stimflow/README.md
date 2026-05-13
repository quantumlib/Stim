stimflow: annealed utilities for creating QEC circuits
=================================================

stimflow is a library for creating quantum error correction circuits.

stimflow's design philosophy is to be a tool box, not a black box.
For example, stimflow does *not* include a `make_surface_code` method.
Instead it provides tools that can be used to more easily create a surface code circuit from scratch.
The hope is that these tools then make it easier to create as-yet-unknown constructions in the future.

stimflow decomposes the circuit creation problem into making and combining *chunks*.
A *Chunk* is a circuit combined with stabilizer flow assertions that the circuit is supposed to satisfy.
stimflow provides tools for making chunks (`stimflow.ChunkBuilder`), verifying chunks (`stimflow.Chunk.verify`), debugging chunks (`stimflow.Chunk.to_html_viewer`), and compiling sequences of chunks into a complete final circuit (`stimflow.ChunkCompiler`).

stimflow also includes functionality for:

- Transpiling (`stimflow.transpile_to_z_basis_interaction_circuit(...)`)
- Adding Noise (`stimflow.NoiseModel.uniform_depolarizing(p).noisy_circuit(...)`)
- Visualizing (`stimflow.make_3d_model`, `stimflow.stim_circuit_html_viewer`)

# Documentation

See stimflow's [getting started notebook](doc/getting_started.ipynb).

See stimflow's [API reference](doc/api.md).

# Backwards Compatibility Warning

Stimflow does not currently guarantee backwards compatibility.
There are parts of the library that do not yet feel like they have converged on the "right" way to do it,
and I want to maintain the freedom to fix them later.

# Example Usage: Surface Code Circuit

stimflow is not yet provided as a pypi package, so you cannot install it with pip.
The installation can be done manually by copying the contents of this directory somewhere into your python path.

The following is python code that emits a surface code circuit.

```python
import stimflow as sf


def make_surface_code(d: int) -> sf.StabilizerCode:
    """Defines the stabilizers and observables of a surface code."""
    tiles = []
    ds = [0, 1, 1j, 1 + 1j]
    for x in range(-1, d):
        for y in range(-1, d):
            m = x + 1j * y
            qs = [m + d for d in ds]
            qs = [q for q in qs
                  if 0 <= q.real < d and 0 <= q.imag < d]
            b = 'XZ'[(x + y) % 2]
            if b == 'X' and x in [-1, d - 1]:
                continue
            if b == 'Z' and y in [-1, d - 1]:
                continue
            tiles.append(sf.Tile(
                data_qubits=qs,
                bases=b,
                measure_qubit=m + 0.5 + 0.5j,
            ))

    patch = sf.Patch(tiles)
    obs_x = sf.PauliMap.from_xs([q for q in patch.data_set if q.real == 0]).with_name('X')
    obs_z = sf.PauliMap.from_zs([q for q in patch.data_set if q.imag == 0]).with_name('Z')
    return sf.StabilizerCode(patch, logicals=[(obs_x, obs_z)])


def make_idle_round(d: int) -> sf.Chunk:
    """Creates a circuit that performs one round of surface code stabilizer measurement."""
    code = make_surface_code(d=d)
    builder = sf.ChunkBuilder(allowed_qubits=code.used_set)
    mxs = [tile.measure_qubit for tile in code.tiles if tile.basis == 'X']
    mzs = [tile.measure_qubit for tile in code.tiles if tile.basis == 'Z']

    # Prepare measure qubits.
    builder.append("RX", mxs)
    builder.append("RZ", mzs)
    builder.append("TICK")

    # Perform entangling gates.
    dxs = [-0.5 - 0.5j, 0.5 - 0.5j, -0.5 + 0.5j, 0.5 + 0.5j]
    dzs = [dxs[0], dxs[2], dxs[1], dxs[3]]
    for k in range(4):
        builder.append(
            'CX',
            [(m, m + dxs[k]) for m in mxs] + [(m + dzs[k], m) for m in mzs],
            unknown_qubit_append_mode='skip',
        )
        builder.append("TICK")

    # Measure the measure qubits.
    builder.append("MX", mxs)
    builder.append("MZ", mzs)

    # Assert the circuit should be preparing and measuring the stabilizers.
    for tile in code.tiles:
        builder.add_flow(start=tile, measurements=[tile.measure_qubit])
        builder.add_flow(end=tile, measurements=[tile.measure_qubit])
    # Assert the circuit should be preserving the logical operators.
    for obs in code.flat_logicals:
        builder.add_flow(start=obs, end=obs)

    return builder.finish_chunk()


def main():
    # Create the code, verify its commutation relationships, and save a picture of it.
    code = make_surface_code(d=7)
    code.verify()
    code.to_svg().write_to('tmp.svg')

    # Create the circuit cycle, verify its operation, and create an interactive viewer.
    chunk = make_idle_round(d=7)
    chunk.to_html_viewer(background=code).write_to('tmp.html')
    chunk.verify()

    # Compile a physical memory experiment with alternating cycle orderings.
    compiler = sf.ChunkCompiler()
    compiler.append(code.transversal_init_chunk(basis='X'))
    compiler.append(sf.ChunkLoop(
        [chunk, chunk.time_reversed()],
        repetitions=5,
    ))
    compiler.append(code.transversal_measure_chunk(basis='X'))
    circuit = compiler.finish_circuit()

    # Add noise to the circuit, check its distance, and make another viewer.
    noisy_circuit = sf.NoiseModel.uniform_depolarizing(1e-3).noisy_circuit(circuit)
    distance = len(noisy_circuit.shortest_graphlike_error())
    assert distance == 7
    sf.stim_circuit_html_viewer(noisy_circuit, background=code).write_to('tmp2.html')


if __name__ == "__main__":
    main()
```
