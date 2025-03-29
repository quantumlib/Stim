# Stim

<img align="right" width="125em" alt="Stim logo" src="https://raw.githubusercontent.com/quantumlib/Stim/refs/heads/main/doc/logo_128x128.svg">

High-performance simulation of quantum stabilizer circuits for quantum error correction.

◼︎︎&nbsp;&nbsp;[What is Stim?](#what-is-stim)<br>
◼︎︎&nbsp;&nbsp;[How do I use Stim?](#how-use-stim)<br>
◼︎&nbsp;&nbsp;[How does Stim work?](#how-stim-work)<br>
◼︎&nbsp;&nbsp;[How do I cite Stim?](#how-cite-stim)<br>
◼︎&nbsp;&nbsp;[*Subproject*: Sinter decoding sampler](glue/sample)<br>
◼︎&nbsp;&nbsp;[*Subproject*: Crumble interactive editor](glue/crumble)<br>

## <a name="what-is-stim"></a>What is Stim?

Stim is a tool for high performance simulation and analysis of quantum stabilizer circuits,
especially quantum error correction (QEC) circuits.
Typically Stim is used as a Python package (`pip install stim`), though Stim can also be used as
a command-line tool or a C++ library.

*   [Watch the 15 minute lightning talk presenting Stim at QPL2021](https://youtu.be/7m_JrJIskPM?t=895)
*   [Watch Stim being used to estimate the threshold of the honeycomb code over a
    weekend](https://www.youtube.com/watch?v=E9yj0o1LGII)

Stim's key features:

1.  **<em>Really</em> fast simulation of stabilizer circuits**.
    Have a circuit with thousands of qubits and millions of operations?
    [`stim.Circuit.compile_sampler()`](doc/python_api_reference_vDev.md#stim.Circuit.compile_sampler) will perform a few
    seconds of analysis and then produce an object that can sample shots at kilohertz rates.

2.  **Semi-automatic decoder configuration**.
    [`stim.Circuit.detector_error_model()`](doc/python_api_reference_vDev.md#stim.Circuit.detector_error_model) converts
    a noisy circuit into a detector error model (a [Tanner graph](https://en.wikipedia.org/wiki/Tanner_graph)) which can
    be used to configure decoders.
    Adding the option `decompose_operations=True` will additionally suggest how hyper errors can be decomposed into
    graphlike errors, making it easier to configure matching-based decoders.

3.  **Useful building blocks for working with stabilizers**, such as
    [`stim.PauliString`](doc/python_api_reference_vDev.md#stim.PauliString),
    [`stim.Tableau`](doc/python_api_reference_vDev.md#stim.Tableau),
    and [`stim.TableauSimulator`](doc/python_api_reference_vDev.md#stim.TableauSimulator).

Stim's main limitations are:

1.  There is no support for non-Clifford operations, such as T gates and Toffoli gates. Only stabilizer operations are
    supported.

2.  `stim.Circuit` only supports Pauli noise channels (eg. no amplitude decay). For more complex noise you must manually
    drive a `stim.TableauSimulator`.

3.  `stim.Circuit` only supports single-control Pauli feedback. For multi-control feedback, or non-Pauli feedback, you
    must manually drive a `stim.TableauSimulator`.

Stim's design philosophy:

*   **Performance is king.**
    The goal is not to be fast *enough*, it is to be fast in an absolute sense.
    Think of it this way.
    The difference between doing one thing per second (human speeds) and doing ten billion things
    per second (computer speeds) is 100 decibels (100 factors of 1.26).
    Because software slowdowns tend to compound exponentially, the choices we make can be thought of multiplicatively;
    they can be thought of as spending or saving decibels.
    For example, under default usage, Python is 100 times slower than C++.
    That's 20dB of the 100dB budget!
    A *fifth* of the multiplicative performance budget allocated to *language choice*!
    Too expensive!
    Although Stim will never achieve the glory of [30 GiB per second of
    FizzBuzz](https://codegolf.stackexchange.com/a/236630/74349), it at least *wishes* it could.

*   **Bottom up.**
    Stim is intended to be like an assembly language: a mostly straightforward layer upon which more complex layers
    can be built.
    The user may define QEC constructions at some high level, perhaps as a set of stabilizers or as a parity check
    matrix, but these concepts are explained to Stim at a low level (e.g., as circuits).
    Stim is not necessarily the abstraction that the user wants, but Stim wants to implement low-level
    pieces simple enough and fast enough that the high-level pieces that the user wants can be built on top.

*   **Backwards compatibility.**
    Stim's Python package uses semantic versioning.
    Within a major version (1.X), Stim guarantees backwards compatibility of its Python API and of its command-line API.
    Note Stim DOESN'T guarantee backwards compatibility of the underlying C++ API.

## <a name="how-use-stim"></a>How do I use Stim?

See the [Getting Started Notebook](doc/getting_started.ipynb).

Stuck?
[Get help on the quantum computing stack exchange](https://quantumcomputing.stackexchange.com)
and use the [`stim`](https://quantumcomputing.stackexchange.com/questions/tagged/stim) tag.

See the reference documentation:

*   [Stim Python API Reference](doc/python_api_reference_vDev.md)
*   [Stim Supported Gates Reference](doc/gates.md)
*   [Stim Command Line Reference](doc/usage_command_line.md)
*   [Stim Circuit File Format (.stim)](doc/file_format_stim_circuit.md)
*   [Stim Detector Error model Format (.dem)](doc/file_format_dem_detector_error_model.md)
*   [Stim Results Format Reference](doc/result_formats.md)
*   [Stim Internal Developer Reference](doc/developer_documentation.md)

## <a name="how-stim-work"></a>How does Stim work?

See [the paper describing Stim](https://quantum-journal.org/papers/q-2021-07-06-497/).
Stim makes three core improvements over previous stabilizer simulators:

1.  **Vectorized code.**
    Stim's hot loops are heavily vectorized, using 256 bit wide AVX instructions.
    This makes them very fast.
    For example, Stim can multiply Pauli strings with 100 billion terms in one second.

2.  **Reference Frame Sampling.**
    When bulk sampling, Stim only uses a general stabilizer simulator for an initial reference sample.
    After that, it cheaply derives as many samples as needed by propagating simulated errors diffed against the
    reference.
    This simple trick is *ridiculously* cheaper than the alternative: constant cost per gate, instead of linear cost
    or even quadratic cost.

3.  **Inverted Stabilizer Tableau.**
    When doing general stabilizer simulation, Stim tracks the inverse of the stabilizer tableau that was historically
    used.
    This has the unexpected benefit of making measurements that commute with the current stabilizers take
    linear time instead of quadratic time. This is beneficial in error correcting codes, because the measurements
    they perform are usually redundant and so commute with the current stabilizers.

## <a name="how-cite-stim"></a>How do I cite Stim?

When using Stim for research, [please cite](https://quantum-journal.org/papers/q-2021-07-06-497/):

```latex
@article{gidney2021stim,
  doi = {10.22331/q-2021-07-06-497},
  url = {https://doi.org/10.22331/q-2021-07-06-497},
  title = {Stim: a fast stabilizer circuit simulator},
  author = {Gidney, Craig},
  journal = {{Quantum}},
  issn = {2521-327X},
  publisher = {{Verein zur F{\"{o}}rderung des Open Access Publizierens
                in den Quantenwissenschaften}},
  volume = 5,
  pages = 497,
  month = jul,
  year = 2021
}
```

## Contact

For any questions or concerns not addressed here, please email quantum-oss-maintainers@google.com.

## Disclaimer

This is not an officially supported Google product. This project is not eligible for the [Google Open Source Software
Vulnerability Rewards Program](https://bughunters.google.com/open-source-security).

Copyright 2025 Google LLC.

<div align="center">
  <a href="https://quantumai.google">
    <img width="15%" alt="Google Quantum AI"
         src="./doc/quantum-ai-vertical.svg">
  </a>
</div>
