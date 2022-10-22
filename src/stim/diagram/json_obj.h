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

#ifndef _STIM_DRAW_JSON_OBJ_H
#define _STIM_DRAW_JSON_OBJ_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

struct JsonObj {
    float num = 0;
    double double_num = 0;
    std::string text;
    std::map<std::string, JsonObj> map;
    std::vector<JsonObj> arr;
    bool boolean = false;
    uint8_t type;

    JsonObj(bool boolean);
    JsonObj(int num);
    JsonObj(size_t num);
    JsonObj(float num);
    JsonObj(double double_num);
    JsonObj(std::string text);
    JsonObj(const char *text);
    JsonObj(std::map<std::string, JsonObj> map);
    JsonObj(std::vector<JsonObj> arr);

    void clear();
    static void write_str(const std::string &s, std::ostream &out);
    void write(std::ostream &out, int64_t indent = INT64_MIN) const;
    std::string str(bool indent = false) const;
};

std::ostream &operator<<(std::ostream &out, const JsonObj &obj);

}  // namespace stim_draw_internal

#endif
