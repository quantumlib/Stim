#ifndef DETECTION_SIMULATOR_H
#define DETECTION_SIMULATOR_H

#include <random>

#include "../circuit.h"
#include "../simd/simd_bit_table.h"

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng);
void detector_samples_out(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, FILE *out,
    SampleFormat format, std::mt19937_64 &rng);

#endif
