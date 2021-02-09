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

#if __AVX2__
#include "simd_compat_avx2.h"
#elif __SSE2__
#include "simd_compat_sse2.h"
#else
#include "simd_compat_polyfill.h"
#endif

#endif
