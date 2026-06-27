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

#include "stim/diagram/ascii_diagram.h"

#include "gtest/gtest.h"

using namespace stim_draw_internal;

TEST(ascii_diagram, basic) {
    AsciiDiagram diagram;
    diagram.add_entry(
        AsciiDiagramEntry{
            AsciiDiagramPos{0, 0, 0.5, 0.5},
            {"ABC"},
        });
    ASSERT_EQ("\n" + diagram.str() + "\n", R"DIAGRAM(
ABC
)DIAGRAM");
    diagram.add_entry(
        AsciiDiagramEntry{
            AsciiDiagramPos{2, 0, 0.5, 0.5},
            {"DE"},
        });
    ASSERT_EQ("\n" + diagram.str() + "\n", R"DIAGRAM(
ABC DE
)DIAGRAM");
    diagram.lines.push_back({
        AsciiDiagramPos{0, 1, 1, 0.5},
        AsciiDiagramPos{2, 1, 0, 0.5},
    });
    diagram.lines.push_back({
        AsciiDiagramPos{0, 2, 0, 0.5},
        AsciiDiagramPos{2, 2, 1, 0.5},
    });
    diagram.lines.push_back({
        AsciiDiagramPos{1, 3, 0, 0.5},
        AsciiDiagramPos{1, 3, 1, 0.5},
    });
    diagram.lines.push_back({
        AsciiDiagramPos{1, 4, 1, 0.5},
        AsciiDiagramPos{1, 4, 0, 0.5},
    });
    ASSERT_EQ("\n" + diagram.str() + "\n", R"DIAGRAM(
ABC DE
   -
------
   -
   -
)DIAGRAM");
}
