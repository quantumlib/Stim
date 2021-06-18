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

#ifndef STIM_CIRCUIT_TEST_H
#define STIM_CIRCUIT_TEST_H

#include "circuit.h"

// Helper class for creating temporary operation data.
struct OpDat {
    std::vector<uint32_t> targets;
    OpDat(uint32_t target);
    OpDat(std::vector<uint32_t> targets);
    static OpDat flipped(size_t target);
    operator stim_internal::OperationData();
};

#endif
