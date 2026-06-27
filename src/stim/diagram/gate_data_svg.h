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

#ifndef _STIM_DIAGRAM_GATE_DATA_SVG_H
#define _STIM_DIAGRAM_GATE_DATA_SVG_H

#include <cstdint>
#include <map>
#include <string>

namespace stim_draw_internal {

struct SvgGateData {
    uint16_t span;
    std::string body;
    std::string subscript;
    std::string superscript;
    std::string fill;
    std::string text_color;
    size_t font_size;
    size_t sub_font_size;
    int32_t y_shift;

    static std::map<std::string_view, SvgGateData> make_gate_data_map();
};

}  // namespace stim_draw_internal

#endif
