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

#ifndef _STIM_PY_SIMULATORS_MEASUREMENTS_TO_DETECTION_EVENTS_PYBIND_H
#define _STIM_PY_SIMULATORS_MEASUREMENTS_TO_DETECTION_EVENTS_PYBIND_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_bits.h"

struct CompiledMeasurementsToDetectionEventsConverter {
    const bool skip_reference_sample;
    const stim::simd_bits ref_sample;
    const stim::Circuit circuit;
    const uint64_t circuit_num_measurements;
    const uint64_t circuit_num_sweep_bits;
    const uint64_t circuit_num_detectors;
    const uint64_t circuit_num_observables;
    const size_t circuit_num_qubits;
    CompiledMeasurementsToDetectionEventsConverter() = delete;
    CompiledMeasurementsToDetectionEventsConverter(const CompiledMeasurementsToDetectionEventsConverter &) = delete;
    CompiledMeasurementsToDetectionEventsConverter(CompiledMeasurementsToDetectionEventsConverter &&) = default;

    CompiledMeasurementsToDetectionEventsConverter(
        stim::simd_bits ref_sample, stim::Circuit circuit, bool skip_reference_sample);

    pybind11::array_t<bool> convert(
        const pybind11::array_t<bool> &measurements,
        const pybind11::array_t<bool> &sweep_bits,
        bool append_observables);
    void convert_file(
        const std::string &measurements_filepath,
        const std::string &measurements_format,
        const char *sweep_bits_filepath,
        const std::string &sweep_bits_format,
        const std::string &detection_events_filepath,
        const std::string &detection_events_format,
        bool append_observables);

    std::string repr() const;
};

pybind11::class_<CompiledMeasurementsToDetectionEventsConverter>
pybind_compiled_measurements_to_detection_events_converter_class(pybind11::module &m);
void pybind_compiled_measurements_to_detection_events_converter_methods(
    pybind11::class_<CompiledMeasurementsToDetectionEventsConverter> &c);
CompiledMeasurementsToDetectionEventsConverter py_init_compiled_measurements_to_detection_events_converter(
    const stim::Circuit &circuit, bool skip_reference_sample);

#endif
