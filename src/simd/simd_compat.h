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

#ifndef SIMD_COMPAT_H
#define SIMD_COMPAT_H

#ifndef SIMD_WIDTH
#error SIMD_WIDTH must be defined. For example, pass -DSIMD_WIDTH=256 as a compiler flag.
#endif

#if SIMD_WIDTH == 256
#include "simd_compat_256.h"
#elif SIMD_WIDTH == 128
#include "simd_compat_128.h"
#elif SIMD_WIDTH == 64
#include "simd_compat_64.h"
#else
#error Unsupported SIMD_WIDTH value. Supported values are 256 (default), 128, 64.
#endif

#endif
