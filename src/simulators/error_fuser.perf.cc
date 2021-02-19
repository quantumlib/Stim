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

#include "error_fuser.h"

#include <unordered_set>

#include "../benchmark_util.h"

BENCHMARK(SparsePauliFrame_apply_CZ_scattered_10) {
    //    SparsePauliFrame s1{
    //        {0, "X"},
    //        {100, "Y"},
    //        {200, "Z"},
    //        {300, "X"},
    //        {400, "Y"},
    //        {500, "Z"},
    //        {600, "X"},
    //        {700, "Y"},
    //        {800, "Z"},
    //        {900, "X"},
    //    };
    //    SparsePauliFrame s2{
    //        {0, "X"},
    //        {100, "X"},
    //        {200, "X"},
    //        {300, "Y"},
    //        {400, "Y"},
    //        {500, "Y"},
    //        {600, "Z"},
    //        {700, "Z"},
    //        {800, "Z"},
    //        {900, "X"},
    //    };
    //    benchmark_go([&]() {
    //        s1.for_each_word(s2, [](size_t k, SparsePauliFrameWord &w1, SparsePauliFrameWord &w2) {
    //            w1.z ^= w2.x;
    //            w2.z ^= w1.x;
    //        });
    //    }).goal_nanos(120).show_rate("Paulis", 10);
}
