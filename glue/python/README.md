# Stim

Stim is a fast simulator for quantum stabilizer circuits.

API references are available on the stim github wiki: https://github.com/quantumlib/stim/wiki

Stim can be installed into a python 3 environment using pip:

```bash
pip install stim
```

Once stim is installed, you can `import stim` and use it.
There are three supported use cases:

1. Interactive simulation with `stim.TableauSimulator`.
2. High speed sampling with samplers compiled from `stim.Circuit`.
3. Independent exploration using `stim.Tableau` and `stim.PauliString`.

## Interactive Simulation

Use `stim.TableauSimulator` to simulate operations one by one while inspecting the results:

```python
import stim

s = stim.TableauSimulator()

# Create a GHZ state.
s.h(0)
s.cnot(0, 1)
s.cnot(0, 2)

# Look at the simulator state re-inverted to be forwards:
t = s.current_inverse_tableau()
print(t**-1)
# prints:
# +-xz-xz-xz-
# | ++ ++ ++
# | ZX _Z _Z
# | _X XZ __
# | _X __ XZ

# Measure the GHZ state.
print(s.measure_many(0, 1, 2))
# prints one of:
# [True, True, True]
# or:
# [False, False, False]
```

## High Speed Sampling

By creating a `stim.Circuit` and compiling it into a sampler, samples can be generated very quickly:

```python
import stim

# Create a circuit that measures a large GHZ state.
c = stim.Circuit()
c.append_operation("H", [0])
for k in range(1, 30):
    c.append_operation("CNOT", [0, k])
c.append_operation("M", range(30))

# Compile the circuit into a high performance sampler.
sampler = c.compile_sampler()

# Collect a batch of samples.
# Note: the ideal batch size, in terms of speed per sample, is roughly 1024.
# Smaller batches are slower because they are not sufficiently vectorized.
# Bigger batches are slower because they use more memory.
batch = sampler.sample(1024)
print(type(batch))  # numpy.ndarray
print(batch.dtype)  # numpy.uint8
print(batch.shape)  # (1024, 30)
print(batch)
# Prints something like:
# [[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  ...
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]]
```

This also works on circuits that include noise:

```python
import stim
import numpy as np

c = stim.Circuit("""
    X_ERROR(0.1) 0
    Y_ERROR(0.2) 1
    Z_ERROR(0.3) 2
    DEPOLARIZE1(0.4) 3
    DEPOLARIZE2(0.5) 4 5
    M 0 1 2 3 4 5
""")
batch = c.compile_sampler().sample(2**20)
print(np.mean(batch, axis=0).round(3))
# Prints something like:
# [0.1   0.2   0.    0.267 0.267 0.266]
```

You can also sample annotated detection events using `stim.Circuit.compile_detector_sampler`.

For a list of gates that can appear in a `stim.Circuit`, see the [latest readme on github](https://github.com/quantumlib/Stim#readme).

## Independent Exploration

Stim provides data types `stim.PauliString` and `stim.Tableau`, which support a variety of fast operations.

```python
import stim

xx = stim.PauliString("XX")
yy = stim.PauliString("YY")
assert xx * yy == -stim.PauliString("ZZ")

s = stim.Tableau.from_named_gate("S")
print(repr(s))
# prints:
# stim.Tableau.from_conjugated_generators(
#     xs=[
#         stim.PauliString("+Y"),
#     ],
#     zs=[
#         stim.PauliString("+Z"),
#     ],
# )

s_dag = stim.Tableau.from_named_gate("S_DAG")
assert s**-1 == s_dag
assert s**1000000003 == s_dag

cnot = stim.Tableau.from_named_gate("CNOT")
cz = stim.Tableau.from_named_gate("CZ")
h = stim.Tableau.from_named_gate("H")
t = stim.Tableau(5)
t.append(cnot, [1, 4])
t.append(h, [4])
t.append(cz, [1, 4])
t.prepend(h, [4])
assert t == stim.Tableau(5)
```
