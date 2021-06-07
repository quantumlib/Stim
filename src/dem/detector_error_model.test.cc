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

#include "detector_error_model.h"
#include "../test_util.test.h"

#include <gtest/gtest.h>

using namespace stim_internal;

TEST(detector_error_model, init_equality) {
    DetectorErrorModel model1;
    DetectorErrorModel model2;
    ASSERT_TRUE(model1 == model2);
    ASSERT_TRUE(!(model1 != model2));
    model1.append_tick(5);
    ASSERT_TRUE(model1 != model2);
    ASSERT_TRUE(!(model1 == model2));
    model2.append_tick(4);
    ASSERT_NE(model1, model2);
    model1.clear();
    model2.clear();
    ASSERT_EQ(model1, model2);

    model1.append_repeat_block(5, {});
    model2.append_repeat_block(4, {});
    ASSERT_NE(model1, model2);

    model1.append_error(5, {});
    model2.append_repeat_block(4, {});
    ASSERT_NE(model1, model2);
}

TEST(detector_error_model, build) {
    DetectorErrorModel model;
    ASSERT_EQ(model.instructions.size(), 0);
    ASSERT_EQ(model.blocks.size(), 0);
    model.append_tick(5);
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.instructions[0].type, DEM_TICK);
    ASSERT_EQ(model.instructions[0].target_data.size(), 1);
    ASSERT_EQ(model.instructions[0].target_data[0].t.data, 5);
    ASSERT_EQ(model.blocks.size(), 0);

    std::vector<DemRelativeSymptom> symptoms;
    symptoms.push_back(DemRelativeSymptom::observable_id(3));
    symptoms.push_back(DemRelativeSymptom::detector_id(DemRelValue::unspecified(), DemRelValue::unspecified(), DemRelValue::absolute(4)));
    model.append_error(0.25, symptoms);
    ASSERT_EQ(model.instructions.size(), 2);
    ASSERT_EQ(model.blocks.size(), 0);
    ASSERT_EQ(model.instructions[1].type, DEM_ERROR);
    ASSERT_EQ(model.instructions[1].target_data, (PointerRange<DemRelativeSymptom>)symptoms);
    ASSERT_EQ(model.instructions[1].probability, 0.25);

    model.clear();
    ASSERT_EQ(model.instructions.size(), 0);

    symptoms.push_back(DemRelativeSymptom::separator());
    symptoms.push_back(DemRelativeSymptom::observable_id(4));
    ASSERT_THROW({
        model.append_error(0.125, symptoms);
    }, std::invalid_argument);

    model.append_reducible_error(0.125, symptoms);
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.blocks.size(), 0);
    ASSERT_EQ(model.instructions[0].type, DEM_REDUCIBLE_ERROR);
    ASSERT_EQ(model.instructions[0].target_data, (PointerRange<DemRelativeSymptom>)symptoms);
    ASSERT_EQ(model.instructions[0].probability, 0.125);

    model.clear();

    DetectorErrorModel block;
    block.append_tick(3);
    DetectorErrorModel block2 = block;

    model.append_repeat_block(5, block);
    block.append_tick(4);
    model.append_repeat_block(6, std::move(block));
    model.append_repeat_block(20, block2);
    ASSERT_EQ(model.instructions.size(), 3);
    ASSERT_EQ(model.blocks.size(), 3);
    ASSERT_EQ(model.instructions[0].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[0].target_data[0].t.data, 5);
    ASSERT_EQ(model.instructions[0].target_data[0].x.data, 0);
    ASSERT_EQ(model.instructions[1].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[1].target_data[0].t.data, 6);
    ASSERT_EQ(model.instructions[1].target_data[0].x.data, 1);
    ASSERT_EQ(model.instructions[2].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[2].target_data[0].t.data, 20);
    ASSERT_EQ(model.instructions[2].target_data[0].x.data, 2);
    ASSERT_EQ(model.blocks[0], block2);
    ASSERT_EQ(model.blocks[2], block2);
    block2.append_tick(4);
    ASSERT_EQ(model.blocks[1], block2);
}

TEST(detector_error_model, round_trip_str) {
    const char *t = R"MODEL(error(0.125) D0
repeat 100 {
    repeat 200 {
        reducible_error(0.25) D0 D1+t,5 L0 ^ D2
        tick 10
    }
    error(0.375) D0 D0,1,2
    tick 20
}
)MODEL";
    ASSERT_EQ(DetectorErrorModel(t).str(), std::string(t));
}

TEST(detector_error_model, parse) {
    DetectorErrorModel expected;
    ASSERT_EQ(DetectorErrorModel(""), expected);

    expected.append_error(0.125, (std::vector<DemRelativeSymptom>{DemRelativeSymptom::detector_id(DemRelValue::unspecified(), DemRelValue::unspecified(), DemRelValue::absolute(0))}));
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
    )MODEL"), expected);

    expected.append_error(0.125, (std::vector<DemRelativeSymptom>{DemRelativeSymptom::detector_id(DemRelValue::unspecified(), DemRelValue::unspecified(), DemRelValue::relative(0))}));
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D0+t
    )MODEL"), expected);

    expected.append_error(0.125, (std::vector<DemRelativeSymptom>{DemRelativeSymptom::detector_id(DemRelValue::absolute(1), DemRelValue::absolute(2), DemRelValue::relative(3))}));
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D0+t
        error(0.125) D1,2,3+t
    )MODEL"), expected);

    expected.append_reducible_error(0.25, (std::vector<DemRelativeSymptom>{DemRelativeSymptom::observable_id(0), DemRelativeSymptom::separator(), DemRelativeSymptom::observable_id(2)}));
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D0+t
        error(0.125) D1,2,3+t
        reducible_error(0.25) L0 ^ L2
    )MODEL"), expected);

    expected.append_tick(60);
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D0+t
        error(0.125) D1,2,3+t
        reducible_error(0.25) L0 ^ L2
        tick 60
    )MODEL"), expected);

    expected.append_repeat_block(100, expected);
    ASSERT_EQ(DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D0+t
        error(0.125) D1,2,3+t
        reducible_error(0.25) L0 ^ L2
        tick 60
        repeat 100 {
            error(0.125) D0
            error(0.125) D0+t
            error(0.125) D1,2,3+t
            reducible_error(0.25) L0 ^ L2
            tick 60
        }
    )MODEL"), expected);
}

TEST(detector_error_model, movement) {
    const char *t = R"MODEL(
        error(0.2) D0
        REPEAT 100 {
            REPEAT 200 {
                reducible_error(0.1) D0 D1+t,5 L0 ^ D2
                TICK 10
            }
            error(0.1) D0 D0,1,2
            TICK 20
        }
    )MODEL";
    DetectorErrorModel d1(t);
    DetectorErrorModel d2(d1);
    ASSERT_EQ(d1, d2);
    ASSERT_EQ(d1, DetectorErrorModel(t));
    DetectorErrorModel d3(std::move(d1));
    ASSERT_EQ(d1, DetectorErrorModel());
    ASSERT_EQ(d2, d3);
    ASSERT_EQ(d2, DetectorErrorModel(t));
    d1 = d3;
    ASSERT_EQ(d1, d2);
    ASSERT_EQ(d2, d3);
    ASSERT_EQ(d2, DetectorErrorModel(t));
    d1 = std::move(d2);
    ASSERT_EQ(d1, d3);
    ASSERT_EQ(d2, DetectorErrorModel());
    ASSERT_EQ(d1, DetectorErrorModel(t));
}


TEST(dem_rel_value, general) {
    auto a = DemRelValue::absolute(3);
    auto u = DemRelValue::unspecified();
    auto r = DemRelValue::relative(3);

    ASSERT_TRUE(!a.is_unspecified());
    ASSERT_TRUE(u.is_unspecified());
    ASSERT_TRUE(!r.is_unspecified());

    ASSERT_TRUE(!a.is_relative());
    ASSERT_TRUE(!u.is_relative());
    ASSERT_TRUE(r.is_relative());

    ASSERT_TRUE(a.is_absolute());
    ASSERT_TRUE(!u.is_absolute());
    ASSERT_TRUE(!r.is_absolute());

    ASSERT_EQ(u.absolute_value(0), 0);
    ASSERT_EQ(u.absolute_value(20), 0);
    ASSERT_EQ(a.absolute_value(0), 3);
    ASSERT_EQ(a.absolute_value(20), 3);
    ASSERT_EQ(r.absolute_value(0), 3);
    ASSERT_EQ(r.absolute_value(20), 23);

    ASSERT_TRUE(a == DemRelValue::absolute(3));
    ASSERT_TRUE(!(a != DemRelValue::absolute(3)));
    ASSERT_TRUE(!(a == DemRelValue::absolute(4)));
    ASSERT_TRUE(a != DemRelValue::absolute(4));

    ASSERT_EQ(u, DemRelValue::unspecified());
    ASSERT_EQ(a, DemRelValue::absolute(3));
    ASSERT_EQ(r, DemRelValue::relative(3));
    ASSERT_NE(u, a);
    ASSERT_NE(r, a);
    ASSERT_NE(u, r);

    ASSERT_EQ(u.raw_value(), 0);
    ASSERT_EQ(a.raw_value(), 3);
    ASSERT_EQ(r.raw_value(), 3);

    ASSERT_EQ(a.str(), "3");
    ASSERT_EQ(r.str(), "3+t");
    ASSERT_EQ(u.str(), "unspecified");
}

TEST(dem_relative_symptom, general) {
    auto u = DemRelValue::unspecified();
    DemRelativeSymptom d = DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(3));
    ASSERT_TRUE(d == DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(3)));
    ASSERT_TRUE(!(d != DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(3))));
    ASSERT_TRUE(!(d == DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(4))));
    ASSERT_TRUE(d != DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(4)));
    ASSERT_EQ(d, DemRelativeSymptom::detector_id(u, u, DemRelValue::relative(3)));
    ASSERT_NE(d, DemRelativeSymptom::detector_id(u, u, u));
    ASSERT_NE(d, DemRelativeSymptom::detector_id(u, DemRelValue::relative(3), DemRelValue::relative(3)));
    ASSERT_NE(d, DemRelativeSymptom::detector_id(DemRelValue::relative(3), u, DemRelValue::relative(3)));
    ASSERT_NE(d, DemRelativeSymptom::observable_id(5));
    ASSERT_NE(d, DemRelativeSymptom::separator());

    DemRelativeSymptom d3 = DemRelativeSymptom::detector_id(DemRelValue::absolute(4), DemRelValue::absolute(6), DemRelValue::relative(3));
    DemRelativeSymptom s = DemRelativeSymptom::separator();
    DemRelativeSymptom o = DemRelativeSymptom::observable_id(3);
    ASSERT_EQ(d.str(), "D3+t");
    ASSERT_EQ(d3.str(), "D4,6,3+t");
    ASSERT_EQ(o.str(), "L3");
    ASSERT_EQ(s.str(), "^");

    ASSERT_TRUE(!o.is_separator());
    ASSERT_TRUE(!d3.is_separator());
    ASSERT_TRUE(s.is_separator());

    ASSERT_TRUE(o.is_observable_id());
    ASSERT_TRUE(!d3.is_observable_id());
    ASSERT_TRUE(!s.is_observable_id());

    ASSERT_TRUE(!o.is_detector_id());
    ASSERT_TRUE(d3.is_detector_id());
    ASSERT_TRUE(!s.is_detector_id());
}

TEST(dem_instruction, general) {
    std::vector<DemRelativeSymptom> d1;
    d1.push_back(DemRelativeSymptom::observable_id(4));
    d1.push_back(DemRelativeSymptom::detector_id(DemRelValue::unspecified(), DemRelValue::unspecified(), DemRelValue::absolute(3)));
    std::vector<DemRelativeSymptom> d2;
    d2.push_back(DemRelativeSymptom::observable_id(4));
    DemInstruction i1{0.125, d1, DEM_ERROR};
    DemInstruction i1a{0.125, d1, DEM_ERROR};
    DemInstruction i2{0.125, d2, DEM_ERROR};
    ASSERT_TRUE(i1 == i1a);
    ASSERT_TRUE(!(i1 != i1a));
    ASSERT_TRUE(!(i2 == i1a));
    ASSERT_TRUE(i2 != i1a);

    ASSERT_EQ(i1, (DemInstruction{0.125, d1, DEM_ERROR}));
    ASSERT_NE(i1, (DemInstruction{0.125, d2, DEM_ERROR}));
    ASSERT_NE(i1, (DemInstruction{0.25, d1, DEM_ERROR}));
    ASSERT_NE(i1, (DemInstruction{0.125, d1, DEM_REDUCIBLE_ERROR}));

    ASSERT_TRUE(i1.approx_equals(DemInstruction{0.125, d1, DEM_ERROR}, 0));
    ASSERT_TRUE(!i1.approx_equals(DemInstruction{0.126, d1, DEM_ERROR}, 0));
    ASSERT_TRUE(i1.approx_equals(DemInstruction{0.126, d1, DEM_ERROR}, 0.01));
    ASSERT_TRUE(!i1.approx_equals(DemInstruction{0.125, d1, DEM_REDUCIBLE_ERROR}, 9999));
    ASSERT_TRUE(!i1.approx_equals(DemInstruction{0.125, d2, DEM_REDUCIBLE_ERROR}, 9999));

    ASSERT_EQ(i1.str(), "error(0.125) L4 D3");
    ASSERT_EQ(i2.str(), "error(0.125) L4");

    d1.push_back(DemRelativeSymptom::separator());
    d1.push_back(DemRelativeSymptom::observable_id(11));
    ASSERT_EQ((DemInstruction{0.25, d1, DEM_REDUCIBLE_ERROR}).str(), "reducible_error(0.25) L4 D3 ^ L11");
    d2.clear();
    d2.push_back({{4}, {5}, {6}});
    ASSERT_EQ((DemInstruction{0, d2, DEM_TICK}).str(), "tick 6");
    ASSERT_EQ((DemInstruction{0, d2, DEM_REPEAT_BLOCK}).str(), "repeat 6 { ... }");
}
