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

#include "stim/mem/sparse_xor_vec.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(SparseXorTable_SmallRowXor_1000) {
    size_t n = 1000;
    std::vector<SparseXorVec<uint32_t>> table(n);
    for (uint32_t k = 0; k < n; k++) {
        table[k].xor_item(k);
        table[k].xor_item(k + 1);
        table[k].xor_item(k + 4);
        table[k].xor_item(k + 8);
        table[k].xor_item(k + 15);
    }

    benchmark_go([&]() {
        for (size_t k = 1; k < n; k++) {
            table[k - 1] ^= table[k];
        }
        for (size_t k = n; --k > 1;) {
            table[k - 1] ^= table[k];
        }
    })
        .goal_micros(35)
        .show_rate("RowXors", n * 2)
        .show_rate("WordXors", n * 3);
}

BENCHMARK(SparseXorVec_XorItem) {
    SparseXorVec<uint32_t> buf;
    std::vector<uint32_t> data{2, 5, 9, 5, 3, 6, 10};

    benchmark_go([&]() {
        for (auto d : data) {
            buf.xor_item(d);
        }
    })
        .goal_nanos(30)
        .show_rate("Item", data.size());
}
