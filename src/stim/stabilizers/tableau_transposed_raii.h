/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_STABILIZERS_TABLEAU_TRANSPOSED_RAII_H
#define _STIM_STABILIZERS_TABLEAU_TRANSPOSED_RAII_H

#include <iostream>
#include <unordered_map>

#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_util.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// When this class is constructed, it transposes the tableau given to it.
/// The transpose is undone on deconstruction.
///
/// This is useful when appending operations to the tableau, since otherwise
/// the append would be working against the grain of memory.
struct TableauTransposedRaii {
    Tableau &tableau;

    explicit TableauTransposedRaii(Tableau &tableau);
    ~TableauTransposedRaii();

    TableauTransposedRaii() = delete;
    TableauTransposedRaii(const TableauTransposedRaii &) = delete;
    TableauTransposedRaii(TableauTransposedRaii &&) = delete;

    PauliString unsigned_x_input(size_t q) const;

    void append_H_XZ(size_t q);
    void append_H_XY(size_t q);
    void append_H_YZ(size_t q);
    void append_S(size_t q);
    void append_ZCX(size_t control, size_t target);
    void append_ZCY(size_t control, size_t target);
    void append_ZCZ(size_t control, size_t target);
    void append_X(size_t q);
    void append_SWAP(size_t q1, size_t q2);
};

}  // namespace stim

#endif
