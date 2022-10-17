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

#ifndef _STIM_DIAGRAM_TIMELINE_IMG_SVG_GATE_DATA_H
#define _STIM_DIAGRAM_TIMELINE_IMG_SVG_GATE_DATA_H

#include <map>
#include <string>

namespace stim_draw_internal {

struct SvgGateData {
    size_t span;
    std::string body;
    std::string subscript;
    std::string superscript;
    std::string fill;

    static std::map<std::string, SvgGateData> make_gate_data_map();
};

}

#endif
