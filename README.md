# What is Stim?

Stim is a tool for high performance simulation and analysis of quantum stabilizer circuits,
intended to help with research into quantum error correcting codes.
Stim can be used as a python package (`pip install stim`),
as a command line tool (built from source in this repo),
or as a C++ library (also built from source in this repo).

- [Watch the 15 minute lightning talk presenting Stim at QPL2021](https://youtu.be/7m_JrJIskPM?t=895).

- [Watch Stim used to estimate the threshold of the honeycomb code within days of it being published](https://www.youtube.com/watch?v=E9yj0o1LGII).

Important Stim features include:

1. **Really** fast simulation of stabilizer circuits.
   As in thousands of times faster than what came before.
   Stim can pull thousands of full shots out of circuits with millions of operations in seconds.
2. Support for annotating noise, detection events, and logical observables directly into circuits.
3. The ability to convert circuits into detector error models,
   which is a convenient format for configuring syndrome decoders.

The main limitations of Stim are:

1. All circuits must be stabilizer circuits (eg. no Toffoli gates).
2. All error channels must be probabilistic Pauli channels (eg. no amplitude decay).
3. All feedback must be Pauli feedback (eg. no classically controlled S gates).

# How do I use Stim?

See the [Getting Started Notebook](doc/getting_started.ipynb).

For using Stim from python, see the [python documentation](glue/python/README.md) and the [python API reference](doc/python_api_reference_vDev.md).
For using Stim from the command line, see the [command line documentation](doc/usage_command_line.md).
For building Stim for yourself, see the [developer documentation](doc/developer_documentation.md).

Circuits are specified using the [stim circuit file format (.stim)](doc/file_format_stim_circuit.md).
See the [supported gate reference](doc/gates.md) for a list of available circuit instructions, and details on what they do.
Samples can be output using [a variety of text and binary formats](doc/result_formats.md).

Error models are specified using the [detector error model file format (.dem)](doc/file_format_dem_detector_error_model.md).

# How does Stim work?

See [the paper describing Stim published in Quantum](https://quantum-journal.org/papers/q-2021-07-06-497/).

In terms of performance, Stim makes three core improvements over previous stabilizer simulators:

1. Stim's hot loops are heavily vectorized, using 256 bit wide AVX instructions.
   This makes them very fast.
   For example, Stim can multiply Pauli strings with 100 billion terms in one second.
2. When bulk sampling, Stim only uses a general stabilizer simulator for an initial reference sample.
   After that, it cheaply derives as many samples as needed by propagating simulated errors diffed against the reference.
   This simple trick is *ridiculously* cheaper than the alternative: constant cost per gate, instead of linear cost or even quadratic cost.
3. When doing general stabilizer simulation, Stim tracks the inverse of the stabilizer tableau that was historically used.
   This has the unexpected benefit of making measurements with a deterministic result (given previous measurements) take
   linear time instead of quadratic time.


# Attribution

When using Stim for research, [please cite](https://quantum-journal.org/papers/q-2021-07-06-497/):

```
@article{gidney2021stim,
  doi = {10.22331/q-2021-07-06-497},
  url = {https://doi.org/10.22331/q-2021-07-06-497},
  title = {Stim: a fast stabilizer circuit simulator},
  author = {Gidney, Craig},
  journal = {{Quantum}},
  issn = {2521-327X},
  publisher = {{Verein zur F{\"{o}}rderung des Open Access Publizierens in den Quantenwissenschaften}},
  volume = {5},
  pages = {497},
  month = jul,
  year = {2021}
}
```
