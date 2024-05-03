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

#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string_iter.h"

namespace stim {

template <size_t W>
PauliStringIterator<W>::PauliStringIterator(
    size_t num_qubits, size_t min_weight, size_t max_weight, bool allow_x, bool allow_y, bool allow_z)
    : num_qubits(num_qubits),
      min_weight(min_weight),
      max_weight(max_weight),
      allow_x(allow_x),
      allow_y(allow_y),
      allow_z(allow_z),
      result(num_qubits) {
    restart();
}

template <size_t W>
bool PauliStringIterator<W>::iter_next() {
    return looper.iter_next([&](size_t loop_index) {
        const NestedLooperLoop &loop = looper.loops[loop_index];
        if (loop_index == 0) {
            // Reached a new weight. Need to iterate over xs.
            looper.loops.resize(loop_index + 1);
            looper.append_combination_loops(num_qubits, loop.cur);
        } else if (loop_index == looper.loops[0].cur) {
            // Reached a new weight mask. Need to iterate over X/Z values.
            looper.loops.resize(loop_index + 1);
            result.xs.clear();
            result.zs.clear();
            size_t pauli_weight = allow_x + allow_y + allow_z;
            for (size_t j = 0; j < looper.loops[0].cur; j++) {
                looper.loops.push_back(NestedLooperLoop{1, 1 + pauli_weight});
            }
        } else if (loop_index > looper.loops[0].cur) {
            // Iterating a pauli. Keep the results up to date as the paulis change.
            auto q = looper.loops[loop_index - looper.loops[0].cur].cur;
            auto v = loop.cur;
            v += !allow_x && v >= 1;
            v += !allow_y && v >= 2;
            v += !allow_z && v >= 3;
            bool y = (v & 1) != 0;
            bool z = (v & 2) != 0;
            result.xs[q] = y ^ z;
            result.zs[q] = z;
        }
    });
}

template <size_t W>
void PauliStringIterator<W>::restart() {
    looper.loops.clear();
    size_t clamped_max_weight = std::min(max_weight, num_qubits);
    if (clamped_max_weight >= min_weight) {
        looper.loops.push_back({min_weight, clamped_max_weight + 1, UINT64_MAX});
    }
    looper.start();
}

}  // namespace stim
