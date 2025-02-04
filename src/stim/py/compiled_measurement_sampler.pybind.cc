// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/py/compiled_measurement_sampler.pybind.h"

#include "stim/circuit/circuit.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;
using namespace stim_pybind;

CompiledMeasurementSampler::CompiledMeasurementSampler(
    simd_bits<MAX_BITWORD_WIDTH> ref_sample, Circuit circuit, bool skip_reference_sample, std::mt19937_64 &&rng)
    : ref_sample(ref_sample), circuit(circuit), skip_reference_sample(skip_reference_sample), rng(std::move(rng)) {
}

pybind11::object CompiledMeasurementSampler::sample_to_numpy(size_t num_shots, bool bit_packed) {
    simd_bit_table<MAX_BITWORD_WIDTH> sample = sample_batch_measurements(circuit, ref_sample, num_shots, rng, false);
    size_t bits_per_sample = circuit.count_measurements();
    return simd_bit_table_to_numpy(sample, bits_per_sample, num_shots, bit_packed, true, pybind11::none());
}

void CompiledMeasurementSampler::sample_write(size_t num_samples, std::string_view filepath, std::string_view format) {
    auto f = format_to_enum(format);
    FILE *out = fopen(std::string(filepath).c_str(), "wb");
    if (out == nullptr) {
        throw std::invalid_argument("Failed to open '" + std::string(filepath) + "' to write.");
    }
    sample_batch_measurements_writing_results_to_disk(circuit, ref_sample, num_samples, out, f, rng);
    fclose(out);
}

std::string CompiledMeasurementSampler::repr() const {
    std::stringstream result;
    result << "stim.CompiledMeasurementSampler(";
    result << circuit_repr(circuit);
    if (skip_reference_sample) {
        result << ", skip_reference_sample=True";
    }
    result << ")";
    return result.str();
}

pybind11::class_<CompiledMeasurementSampler> stim_pybind::pybind_compiled_measurement_sampler(pybind11::module &m) {
    return pybind11::class_<CompiledMeasurementSampler>(
        m, "CompiledMeasurementSampler", "An analyzed stabilizer circuit whose measurements can be sampled quickly.");
}

CompiledMeasurementSampler stim_pybind::py_init_compiled_sampler(
    const Circuit &circuit,
    bool skip_reference_sample,
    const pybind11::object &seed,
    const pybind11::object &reference_sample) {
    if (reference_sample.is_none()) {
        simd_bits<MAX_BITWORD_WIDTH> ref_sample =
            skip_reference_sample ? simd_bits<MAX_BITWORD_WIDTH>(circuit.count_measurements())
                                  : TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(circuit);
        return CompiledMeasurementSampler(ref_sample, circuit, skip_reference_sample, make_py_seeded_rng(seed));
    } else {
        if (skip_reference_sample) {
            throw std::invalid_argument("skip_reference_sample = True but reference_sample is not None.");
        }
        uint64_t num_bits = circuit.count_measurements();
        simd_bits<MAX_BITWORD_WIDTH> ref_sample(num_bits);
        simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample_ref(ref_sample);
        memcpy_bits_from_numpy_to_simd(num_bits, reference_sample, ref_sample_ref);
        return CompiledMeasurementSampler(ref_sample, circuit, skip_reference_sample, make_py_seeded_rng(seed));
    }
}

void stim_pybind::pybind_compiled_measurement_sampler_methods(
    pybind11::module &m, pybind11::class_<CompiledMeasurementSampler> &c) {
    c.def(
        pybind11::init(&py_init_compiled_sampler),
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("skip_reference_sample") = false,
        pybind11::arg("seed") = pybind11::none(),
        pybind11::arg("reference_sample") = pybind11::none(),
        clean_doc_string(R"DOC(
            Creates a measurement sampler for the given circuit.

            The sampler uses a noiseless reference sample, collected from the circuit using
            stim's Tableau simulator during initialization of the sampler, as a baseline for
            deriving more samples using an error propagation simulator.

            Args:
                circuit: The stim circuit to sample from.
                skip_reference_sample: Defaults to False. When set to True, the reference
                    sample used by the sampler is initialized to all-zeroes instead of being
                    collected from the circuit. This means that the results returned by the
                    sampler are actually whether or not each measurement was *flipped*,
                    instead of true measurement results.

                    Forcing an all-zero reference sample is useful when you are only
                    interested in error propagation and don't want to have to deal with the
                    fact that some measurements want to be On when no errors occur. It is
                    also useful when you know for sure that the all-zero result is actually
                    a possible result from the circuit (under noiseless execution), meaning
                    it is a valid reference sample as good as any other. Computing the
                    reference sample is the most time consuming and memory intensive part of
                    simulating the circuit, so promising that the simulator can safely skip
                    that step is an effective optimization.
                seed: PARTIALLY determines simulation results by deterministically seeding
                    the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng is seeded from system entropy.

                    When set to an integer, making the exact same series calls on the exact
                    same machine with the exact same version of Stim will produce the exact
                    same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of
                    Stim. This restriction is present to make it possible to have future
                    optimizations to the random sampling, and is enforced by introducing
                    intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that
                    differ in the width of supported SIMD instructions. For example, using
                    the same seed on a machine that supports AVX instructions and one that
                    only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many
                    shots are taken. For example, taking 10 shots and then 90 shots will
                    give different results from taking 100 shots in one call.
                reference_sample: The data to xor into the measurement flips produced by the
                    frame simulator, in order to produce proper measurement results.
                    This can either be specified as an `np.bool_` array or a bit packed
                    `np.uint8` array (little endian). Under normal conditions, the reference
                    sample should be a valid noiseless sample of the circuit, such as the
                    one returned by `circuit.reference_sample()`. If this argument is not
                    provided, the reference sample will be set to
                    `circuit.reference_sample()`, unless `skip_reference_sample=True`
                    is used, in which case it will be set to all-zeros.

            Returns:
                An initialized stim.CompiledMeasurementSampler.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0   2 3
                ...    M 0 1 2 3
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[ True, False,  True,  True]])
        )DOC")
            .data());

    c.def(
        "sample",
        [](CompiledMeasurementSampler &self, size_t shots, bool bit_packed) {
            return self.sample_to_numpy(shots, bit_packed);
        },
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def sample(self, shots: int, *, bit_packed: bool = False) -> np.ndarray:
            Samples a batch of measurement samples from the circuit.

            Args:
                shots: The number of times to sample every measurement in the circuit.
                bit_packed: Returns a uint8 numpy array with 8 bits per byte, instead of
                    a bool_ numpy array with 1 bit per byte. Uses little endian packing.

            Returns:
                A numpy array containing the samples.

                If bit_packed=False:
                    dtype=bool_
                    shape=(shots, circuit.num_measurements)
                    The bit for measurement `m` in shot `s` is at
                        result[s, m]
                If bit_packed=True:
                    dtype=uint8
                    shape=(shots, math.ceil(circuit.num_measurements / 8))
                    The bit for measurement `m` in shot `s` is at
                        (result[s, m // 8] >> (m % 8)) & 1

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0   2 3
                ...    M 0 1 2 3
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample(shots=1)
                array([[ True, False,  True,  True]])
        )DOC")
            .data());

    c.def(
        "sample_bit_packed",
        [](CompiledMeasurementSampler &self, size_t shots) {
            return self.sample_to_numpy(shots, true);
        },
        pybind11::arg("shots"),
        clean_doc_string(R"DOC(
            [DEPRECATED] Use sampler.sample(..., bit_packed=True) instead.
            @signature def sample_bit_packed(self, shots: int) -> np.ndarray:

            Samples a bit packed batch of measurement samples from the circuit.

            Args:
                shots: The number of times to sample every measurement in the circuit.

            Returns:
                A numpy array with `dtype=uint8` and
                `shape=(shots, (num_measurements + 7) // 8)`.

                The bit for measurement `m` in shot `s` is at
                `result[s, (m // 8)] & 2**(m % 8)`.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0 1 2 3 4 5 6 7     10
                ...    M 0 1 2 3 4 5 6 7 8 9 10
                ... ''')
                >>> s = c.compile_sampler()
                >>> s.sample_bit_packed(shots=1)
                array([[255,   4]], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "sample_write",
        &CompiledMeasurementSampler::sample_write,
        pybind11::arg("shots"),
        pybind11::kw_only(),
        pybind11::arg("filepath"),
        pybind11::arg("format") = "01",
        clean_doc_string(R"DOC(
            Samples measurements from the circuit and writes them to a file.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = f"{d}/tmp.dat"
                ...     c = stim.Circuit('''
                ...         X 0   2 3
                ...         M 0 1 2 3
                ...     ''')
                ...     c.compile_sampler().sample_write(5, filepath=path, format="01")
                ...     with open(path) as f:
                ...         print(f.read(), end='')
                1011
                1011
                1011
                1011
                1011

            Args:
                shots: The number of times to sample every measurement in the circuit.
                filepath: The file to write the results to.
                format: The output format to write the results with.
                    Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
                    Defaults to "01".

            Returns:
                None.
        )DOC")
            .data());

    c.def(
        "__repr__",
        &CompiledMeasurementSampler::repr,
        "Returns text that is a valid python expression evaluating to an equivalent "
        "`stim.CompiledMeasurementSampler`.");
}
