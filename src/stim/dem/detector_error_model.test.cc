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

#include "stim/dem/detector_error_model.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(detector_error_model, init_equality) {
    DetectorErrorModel model1;
    DetectorErrorModel model2;
    ASSERT_TRUE(model1 == model2);
    ASSERT_TRUE(!(model1 != model2));
    model1.append_shift_detectors_instruction({}, 5);
    ASSERT_TRUE(model1 != model2);
    ASSERT_TRUE(!(model1 == model2));
    model2.append_shift_detectors_instruction({}, 4);
    ASSERT_NE(model1, model2);
    model1.clear();
    model2.clear();
    ASSERT_EQ(model1, model2);

    model1.append_repeat_block(5, {});
    model2.append_repeat_block(4, {});
    ASSERT_NE(model1, model2);

    model1.append_error_instruction(0.2, {});
    model2.append_repeat_block(4, {});
    ASSERT_NE(model1, model2);
}

TEST(detector_error_model, append_shift_detectors_instruction) {
    DetectorErrorModel model;
    ASSERT_EQ(model.instructions.size(), 0);
    ASSERT_EQ(model.blocks.size(), 0);

    std::vector<double> arg_data{1.5, 2.5};
    ConstPointerRange<double> arg_data_ref = arg_data;
    model.append_shift_detectors_instruction(arg_data_ref, 5);
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.instructions[0].type, DEM_SHIFT_DETECTORS);
    ASSERT_EQ(model.instructions[0].target_data.size(), 1);
    ASSERT_EQ(model.instructions[0].target_data[0].data, 5);
    ASSERT_EQ(model.instructions[0].arg_data, arg_data_ref);
    ASSERT_EQ(model.blocks.size(), 0);
}

TEST(detector_error_model, append_detector_instruction) {
    DetectorErrorModel model;
    ASSERT_EQ(model.instructions.size(), 0);
    ASSERT_EQ(model.blocks.size(), 0);

    std::vector<double> arg_data{1.5, 2.5};
    ConstPointerRange<double> arg_data_ref = arg_data;
    model.append_detector_instruction(arg_data_ref, DemTarget::relative_detector_id(5));
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.instructions[0].type, DEM_DETECTOR);
    ASSERT_EQ(model.instructions[0].target_data.size(), 1);
    ASSERT_EQ(model.instructions[0].target_data[0], DemTarget::relative_detector_id(5));
    ASSERT_EQ(model.instructions[0].arg_data, arg_data_ref);
    ASSERT_EQ(model.blocks.size(), 0);

    ASSERT_THROW({ model.append_detector_instruction({}, DemTarget::separator()); }, std::invalid_argument);
    ASSERT_THROW({ model.append_detector_instruction({}, DemTarget::observable_id(4)); }, std::invalid_argument);
    model.append_detector_instruction({}, DemTarget::relative_detector_id(4));
}

TEST(detector_error_model, append_logical_observable_instruction) {
    DetectorErrorModel model;
    ASSERT_EQ(model.instructions.size(), 0);
    ASSERT_EQ(model.blocks.size(), 0);

    model.append_logical_observable_instruction(DemTarget::observable_id(5));
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.instructions[0].type, DEM_LOGICAL_OBSERVABLE);
    ASSERT_EQ(model.instructions[0].target_data.size(), 1);
    ASSERT_EQ(model.instructions[0].target_data[0], DemTarget::observable_id(5));
    ASSERT_EQ(model.instructions[0].arg_data.size(), 0);
    ASSERT_EQ(model.blocks.size(), 0);

    ASSERT_THROW({ model.append_logical_observable_instruction(DemTarget::separator()); }, std::invalid_argument);
    ASSERT_THROW(
        { model.append_logical_observable_instruction(DemTarget::relative_detector_id(4)); }, std::invalid_argument);
    model.append_logical_observable_instruction(DemTarget::observable_id(4));
}

TEST(detector_error_model, append_error_instruction) {
    DetectorErrorModel model;
    std::vector<DemTarget> symptoms;
    symptoms.push_back(DemTarget::observable_id(3));
    symptoms.push_back(DemTarget::relative_detector_id(4));
    model.append_error_instruction(0.25, symptoms);
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.blocks.size(), 0);
    ASSERT_EQ(model.instructions[0].type, DEM_ERROR);
    ASSERT_EQ(model.instructions[0].target_data, (PointerRange<DemTarget>)symptoms);
    ASSERT_EQ(model.instructions[0].arg_data.size(), 1);
    ASSERT_EQ(model.instructions[0].arg_data[0], 0.25);

    model.clear();
    ASSERT_EQ(model.instructions.size(), 0);

    symptoms.push_back(DemTarget::separator());
    symptoms.push_back(DemTarget::observable_id(4));

    model.append_error_instruction(0.125, symptoms);
    ASSERT_EQ(model.instructions.size(), 1);
    ASSERT_EQ(model.blocks.size(), 0);
    ASSERT_EQ(model.instructions[0].type, DEM_ERROR);
    ASSERT_EQ(model.instructions[0].target_data, (PointerRange<DemTarget>)symptoms);
    ASSERT_EQ(model.instructions[0].arg_data.size(), 1);
    ASSERT_EQ(model.instructions[0].arg_data[0], 0.125);

    ASSERT_THROW({ model.append_error_instruction(1.5, symptoms); }, std::invalid_argument);
    ASSERT_THROW({ model.append_error_instruction(-0.5, symptoms); }, std::invalid_argument);

    symptoms = {DemTarget::separator()};
    ASSERT_THROW({ model.append_error_instruction(0.25, symptoms); }, std::invalid_argument);
    symptoms = {DemTarget::separator(), DemTarget::observable_id(0)};
    ASSERT_THROW({ model.append_error_instruction(0.25, symptoms); }, std::invalid_argument);
    symptoms = {DemTarget::observable_id(0), DemTarget::separator()};
    ASSERT_THROW({ model.append_error_instruction(0.25, symptoms); }, std::invalid_argument);
    symptoms = {
        DemTarget::observable_id(0),
        DemTarget::separator(),
        DemTarget::separator(),
        DemTarget::relative_detector_id(4)};
    ASSERT_THROW({ model.append_error_instruction(0.25, symptoms); }, std::invalid_argument);
    symptoms = {DemTarget::observable_id(0), DemTarget::separator(), DemTarget::relative_detector_id(4)};
    model.append_error_instruction(0.25, symptoms);
}

TEST(detector_error_model, append_block) {
    DetectorErrorModel model;
    DetectorErrorModel block;
    block.append_shift_detectors_instruction({}, 3);
    DetectorErrorModel block2 = block;

    model.append_repeat_block(5, block);
    block.append_shift_detectors_instruction({}, 4);
    model.append_repeat_block(6, std::move(block));
    model.append_repeat_block(20, block2);
    ASSERT_EQ(model.instructions.size(), 3);
    ASSERT_EQ(model.blocks.size(), 3);
    ASSERT_EQ(model.instructions[0].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[0].target_data[0].data, 5);
    ASSERT_EQ(model.instructions[0].target_data[1].data, 0);
    ASSERT_EQ(model.instructions[1].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[1].target_data[0].data, 6);
    ASSERT_EQ(model.instructions[1].target_data[1].data, 1);
    ASSERT_EQ(model.instructions[2].type, DEM_REPEAT_BLOCK);
    ASSERT_EQ(model.instructions[2].target_data[0].data, 20);
    ASSERT_EQ(model.instructions[2].target_data[1].data, 2);
    ASSERT_EQ(model.blocks[0], block2);
    ASSERT_EQ(model.blocks[2], block2);
    block2.append_shift_detectors_instruction({}, 4);
    ASSERT_EQ(model.blocks[1], block2);
}

TEST(detector_error_model, round_trip_str) {
    const char *t = R"MODEL(error(0.125) D0
repeat 100 {
    repeat 200 {
        error(0.25) D0 D1 L0 ^ D2
        shift_detectors(1.5, 3) 10
        detector(0.5) D0
        detector D1
    }
    error(0.375) D0 D1
    shift_detectors 20
    logical_observable L0
})MODEL";
    ASSERT_EQ(DetectorErrorModel(t).str(), std::string(t));
}

TEST(detector_error_model, parse) {
    DetectorErrorModel expected;
    ASSERT_EQ(DetectorErrorModel(""), expected);

    expected.append_error_instruction(0.125, (std::vector<DemTarget>{DemTarget::relative_detector_id(0)}));
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        error(0.125) D0
    )MODEL"),
        expected);

    expected.append_error_instruction(0.125, (std::vector<DemTarget>{DemTarget::relative_detector_id(5)}));
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D5
    )MODEL"),
        expected);

    expected.append_error_instruction(
        0.25,
        (std::vector<DemTarget>{
            DemTarget::relative_detector_id(5), DemTarget::separator(), DemTarget::observable_id(4)}));
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D5
        error(0.25) D5 ^ L4
    )MODEL"),
        expected);

    expected.append_shift_detectors_instruction(std::vector<double>{1.5, 2}, 60);
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D5
        error(0.25) D5 ^ L4
        shift_detectors(1.5, 2) 60
    )MODEL"),
        expected);

    expected.append_repeat_block(100, expected);
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        error(0.125) D0
        error(0.125) D5
        error(0.25) D5 ^ L4
        shift_detectors(1.5, 2) 60
        repeat 100 {
            error(0.125) D0
            error(0.125) D5
            error(0.25) D5 ^ L4
            shift_detectors(1.5, 2) 60
        }
    )MODEL"),
        expected);
}

TEST(detector_error_model, movement) {
    const char *t = R"MODEL(
        error(0.2) D0
        REPEAT 100 {
            REPEAT 200 {
                error(0.1) D0 D1 L0 ^ D2
                shift_detectors 10
            }
            error(0.1) D0 D2
            shift_detectors 20
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

TEST(dem_target, general) {
    DemTarget d = DemTarget::relative_detector_id(3);
    ASSERT_TRUE(d == DemTarget::relative_detector_id(3));
    ASSERT_TRUE(!(d != DemTarget::relative_detector_id(3)));
    ASSERT_TRUE(!(d == DemTarget::relative_detector_id(4)));
    ASSERT_TRUE(d != DemTarget::relative_detector_id(4));
    ASSERT_EQ(d, DemTarget::relative_detector_id(3));
    ASSERT_NE(d, DemTarget::observable_id(5));
    ASSERT_NE(d, DemTarget::separator());

    DemTarget d3 = DemTarget::relative_detector_id(72);
    DemTarget s = DemTarget::separator();
    DemTarget o = DemTarget::observable_id(3);
    ASSERT_EQ(d.str(), "D3");
    ASSERT_EQ(d3.str(), "D72");
    ASSERT_EQ(o.str(), "L3");
    ASSERT_EQ(s.str(), "^");

    ASSERT_TRUE(!o.is_separator());
    ASSERT_TRUE(!d3.is_separator());
    ASSERT_TRUE(s.is_separator());

    ASSERT_TRUE(o.is_observable_id());
    ASSERT_TRUE(!d3.is_observable_id());
    ASSERT_TRUE(!s.is_observable_id());

    ASSERT_TRUE(!o.is_relative_detector_id());
    ASSERT_TRUE(d3.is_relative_detector_id());
    ASSERT_TRUE(!s.is_relative_detector_id());
}

TEST(dem_instruction, general) {
    std::vector<DemTarget> d1;
    d1.push_back(DemTarget::observable_id(4));
    d1.push_back(DemTarget::relative_detector_id(3));
    std::vector<DemTarget> d2;
    d2.push_back(DemTarget::observable_id(4));
    std::vector<double> p125{0.125};
    std::vector<double> p25{0.25};
    std::vector<double> p126{0.126};
    DemInstruction i1{p125, d1, DEM_ERROR};
    DemInstruction i1a{p125, d1, DEM_ERROR};
    DemInstruction i2{p125, d2, DEM_ERROR};
    ASSERT_TRUE(i1 == i1a);
    ASSERT_TRUE(!(i1 != i1a));
    ASSERT_TRUE(!(i2 == i1a));
    ASSERT_TRUE(i2 != i1a);

    ASSERT_EQ(i1, (DemInstruction{p125, d1, DEM_ERROR}));
    ASSERT_NE(i1, (DemInstruction{p125, d2, DEM_ERROR}));
    ASSERT_NE(i1, (DemInstruction{p25, d1, DEM_ERROR}));
    ASSERT_NE(((DemInstruction{{}, {}, DEM_DETECTOR})), (DemInstruction{{}, {}, DEM_LOGICAL_OBSERVABLE}));

    ASSERT_TRUE(i1.approx_equals(DemInstruction{p125, d1, DEM_ERROR}, 0));
    ASSERT_TRUE(!i1.approx_equals(DemInstruction{p126, d1, DEM_ERROR}, 0));
    ASSERT_TRUE(i1.approx_equals(DemInstruction{p126, d1, DEM_ERROR}, 0.01));
    ASSERT_TRUE(!i1.approx_equals(DemInstruction{p125, d2, DEM_ERROR}, 9999));

    ASSERT_EQ(i1.str(), "error(0.125) L4 D3");
    ASSERT_EQ(i2.str(), "error(0.125) L4");

    d1.push_back(DemTarget::separator());
    d1.push_back(DemTarget::observable_id(11));
    ASSERT_EQ((DemInstruction{p25, d1, DEM_ERROR}).str(), "error(0.25) L4 D3 ^ L11");
}

TEST(detector_error_model, total_detector_shift) {
    ASSERT_EQ(DetectorErrorModel("").total_detector_shift(), 0);
    ASSERT_EQ(DetectorErrorModel("error(0.3) D2").total_detector_shift(), 0);
    ASSERT_EQ(DetectorErrorModel("shift_detectors 5").total_detector_shift(), 5);
    ASSERT_EQ(DetectorErrorModel("shift_detectors 5\nshift_detectors 4").total_detector_shift(), 9);
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        shift_detectors 5
        repeat 1000 {
            shift_detectors 4
        }
    )MODEL")
            .total_detector_shift(),
        4005);
}

TEST(detector_error_model, count_detectors) {
    ASSERT_EQ(DetectorErrorModel("").count_detectors(), 0);
    ASSERT_EQ(DetectorErrorModel("error(0.3) D2 L1000").count_detectors(), 3);
    ASSERT_EQ(DetectorErrorModel("shift_detectors 5").count_detectors(), 0);
    ASSERT_EQ(DetectorErrorModel("shift_detectors 5\ndetector D3").count_detectors(), 9);
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        shift_detectors 50
        repeat 1000 {
            detector D0
            error(0.1) D0 D1
            shift_detectors 4
        }
    )MODEL")
            .count_detectors(),
        4048);
}

TEST(detector_error_model, count_observables) {
    ASSERT_EQ(DetectorErrorModel("").count_observables(), 0);
    ASSERT_EQ(DetectorErrorModel("error(0.3) L2 D9999").count_observables(), 3);
    ASSERT_EQ(DetectorErrorModel("shift_detectors 5\nlogical_observable L3").count_observables(), 4);
    ASSERT_EQ(
        DetectorErrorModel(R"MODEL(
        shift_detectors 50
        repeat 1000 {
            logical_observable L5
            error(0.1) D0 D1 L6
            shift_detectors 4
        }
    )MODEL")
            .count_observables(),
        7);
}

TEST(detector_error_model, from_file) {
    FILE *f = tmpfile();
    const char *program = R"MODEL(
        error(0.125) D1
        REPEAT 99 {
            error(0.25) D3 D4
            shift_detectors 1
        }
    )MODEL";
    fprintf(f, "%s", program);
    rewind(f);
    auto d = DetectorErrorModel::from_file(f);
    ASSERT_EQ(d, DetectorErrorModel(program));
    d.clear();
    rewind(f);
    d.append_from_file(f);
    ASSERT_EQ(d, DetectorErrorModel(program));
    d.clear();
    rewind(f);
    d.append_from_file(f, false);
    ASSERT_EQ(d, DetectorErrorModel(program));
    d.clear();
    rewind(f);
    d.append_from_file(f, true);
    ASSERT_EQ(d, DetectorErrorModel("error(0.125) D1"));
    d.append_from_file(f, true);
    ASSERT_EQ(d, DetectorErrorModel(program));
}

TEST(detector_error_model, py_get_slice) {
    DetectorErrorModel d(R"MODEL(
        detector D2
        logical_observable L1
        error(0.125) D0 L1
        REPEAT 100 {
            shift_detectors(0.25) 5
            REPEAT 20 {
            }
        }
        error(0.125) D1 D2
        REPEAT 999 {
        }
    )MODEL");
    ASSERT_EQ(d.py_get_slice(0, 1, 6), d);
    ASSERT_EQ(d.py_get_slice(0, 1, 4), DetectorErrorModel(R"MODEL(
        detector D2
        logical_observable L1
        error(0.125) D0 L1
        REPEAT 100 {
            shift_detectors(0.25) 5
            REPEAT 20 {
            }
        }
    )MODEL"));
    ASSERT_EQ(d.py_get_slice(2, 1, 3), DetectorErrorModel(R"MODEL(
        error(0.125) D0 L1
        REPEAT 100 {
            shift_detectors(0.25) 5
            REPEAT 20 {
            }
        }
        error(0.125) D1 D2
    )MODEL"));

    ASSERT_EQ(d.py_get_slice(4, -1, 3), DetectorErrorModel(R"MODEL(
        error(0.125) D1 D2
        REPEAT 100 {
            shift_detectors(0.25) 5
            REPEAT 20 {
            }
        }
        error(0.125) D0 L1
    )MODEL"));

    ASSERT_EQ(d.py_get_slice(5, -2, 3), DetectorErrorModel(R"MODEL(
        REPEAT 999 {
        }
        REPEAT 100 {
            shift_detectors(0.25) 5
            REPEAT 20 {
            }
        }
        logical_observable L1
    )MODEL"));

    DetectorErrorModel d2 = d;
    DetectorErrorModel d3 = d2.py_get_slice(0, 1, 6);
    d2.clear();
    ASSERT_EQ(d, d3);
}

TEST(detector_error_model, mul) {
    DetectorErrorModel original(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
    )MODEL");
    DetectorErrorModel d = original;
    ASSERT_EQ(d * 3, DetectorErrorModel(R"MODEL(
        REPEAT 3 {
            error(0.25) D0
            REPEAT 999 {
                error(0.25) D1
            }
        }
    )MODEL"));
    ASSERT_EQ(d * 1, d);
    ASSERT_EQ(d * 0, DetectorErrorModel());
    ASSERT_EQ(d, original);
}

TEST(detector_error_model, imul) {
    DetectorErrorModel original(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
    )MODEL");
    DetectorErrorModel d = original;
    d *= 3;
    ASSERT_EQ(d, DetectorErrorModel(R"MODEL(
        REPEAT 3 {
            error(0.25) D0
            REPEAT 999 {
                error(0.25) D1
            }
        }
    )MODEL"));
    d = original;
    d *= 1;
    ASSERT_EQ(d, original);
    d = original;
    d *= 0;
    ASSERT_EQ(d, DetectorErrorModel());
}

TEST(detector_error_model, add) {
    DetectorErrorModel a(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
    )MODEL");
    DetectorErrorModel b(R"MODEL(
        error(0.125) D1
        REPEAT 2 {
            REPEAT 3 {
                error(0.125) D1
            }
        }
    )MODEL");

    ASSERT_EQ(a + b, DetectorErrorModel(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
        error(0.125) D1
        REPEAT 2 {
            REPEAT 3 {
                error(0.125) D1
            }
        }
    )MODEL"));

    ASSERT_EQ(a + DetectorErrorModel(), a);
    ASSERT_EQ(DetectorErrorModel() + a, a);
    ASSERT_EQ(b + DetectorErrorModel(), b);
    ASSERT_EQ(DetectorErrorModel() + b, b);
    ASSERT_EQ(DetectorErrorModel() + DetectorErrorModel(), DetectorErrorModel());
}

TEST(detector_error_model, iadd) {
    DetectorErrorModel a(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
    )MODEL");
    DetectorErrorModel b(R"MODEL(
        error(0.125) D1
        REPEAT 2 {
            REPEAT 3 {
                error(0.125) D1
            }
        }
    )MODEL");

    a += b;
    ASSERT_EQ(a, DetectorErrorModel(R"MODEL(
        error(0.25) D0
        REPEAT 999 {
            error(0.25) D1
        }
        error(0.125) D1
        REPEAT 2 {
            REPEAT 3 {
                error(0.125) D1
            }
        }
    )MODEL"));

    DetectorErrorModel original = b;
    b += DetectorErrorModel();
    ASSERT_EQ(b, original);
    b += a;
    ASSERT_NE(b, original);

    // Aliased.
    a = original;
    a += a;
    a = DetectorErrorModel(a.str().data());  // Remove memory deduplication, because it affects equality.
    ASSERT_EQ(a, original + original);
}

TEST(detector_error_model, iter_flatten_error_instructions) {
    DetectorErrorModel d(R"MODEL(
        error(0.25) D0
        shift_detectors 1
        error(0.375) D0 D1
        repeat 5 {
            error(0.125) D0 D1 D2 L0
            shift_detectors 2
        }
        detector D5000
        logical_observable L5000
    )MODEL");

    DetectorErrorModel dem;
    d.iter_flatten_error_instructions([&](const DemInstruction &e) {
        EXPECT_EQ(e.type, DEM_ERROR);
        dem.append_error_instruction(e.arg_data[0], e.target_data);
    });
    ASSERT_EQ(dem, DetectorErrorModel(R"MODEL(
        error(0.25) D0
        error(0.375) D1 D2
        error(0.125) D1 D2 D3 L0
        error(0.125) D3 D4 D5 L0
        error(0.125) D5 D6 D7 L0
        error(0.125) D7 D8 D9 L0
        error(0.125) D9 D10 D11 L0
    )MODEL"));
}

TEST(detector_error_model, get_detector_coordinates_nested_loops) {
    DetectorErrorModel dem(R"MODEL(
        repeat 200 {
            repeat 100 {
                detector(0, 0, 0, 4) D1
                shift_detectors(1, 0, 0) 10
            }
            detector(0, 0, 0, 3) D2
            shift_detectors(0, 1, 0) 0
        }
        detector(0, 0, 0, 2) D3
    )MODEL");
    ASSERT_THROW({ dem.get_detector_coordinates({4000000000}); }, std::invalid_argument);
    ASSERT_THROW({ dem.get_detector_coordinates({dem.count_detectors()}); }, std::invalid_argument);
    auto result = dem.get_detector_coordinates({
        0,
        1,
        11,
        991,
        1001,
        1002,
        1011,
        1021,
    });
    ASSERT_EQ(
        result,
        (std::map<uint64_t, std::vector<double>>{
            {0, {}},
            {1, {0, 0, 0, 4}},
            {11, {1, 0, 0, 4}},
            {991, {99, 0, 0, 4}},
            {1001, {100, 1, 0, 4}},
            {1002, {100, 0, 0, 3}},
            {1011, {101, 1, 0, 4}},
            {1021, {102, 1, 0, 4}},
        }));
}

TEST(detector_error_model, get_detector_coordinates_trivial) {
    DetectorErrorModel dem;

    dem = DetectorErrorModel(R"MODEL(
        detector(1, 2) D1
    )MODEL");
    ASSERT_EQ(
        dem.get_detector_coordinates({0, 1}),
        (std::map<uint64_t, std::vector<double>>{
            {0, {}},
            {1, {1, 2}},
        }));
    ASSERT_THROW({ dem.get_detector_coordinates({2}); }, std::invalid_argument);

    dem = DetectorErrorModel(R"MODEL(
        error(0.25) D0 D1
    )MODEL");
    ASSERT_EQ(
        dem.get_detector_coordinates({0, 1}),
        (std::map<uint64_t, std::vector<double>>{
            {0, {}},
            {1, {}},
        }));
    ASSERT_THROW({ dem.get_detector_coordinates({2}); }, std::invalid_argument);

    dem = DetectorErrorModel(R"MODEL(
        error(0.25) D0 D1
        detector(1, 2, 3) D1
        shift_detectors(5) 1
        detector(1, 2) D2
    )MODEL");
    ASSERT_EQ(
        dem.get_detector_coordinates({0, 1, 2, 3}),
        (std::map<uint64_t, std::vector<double>>{
            {0, {}},
            {1, {1, 2, 3}},
            {2, {}},
            {3, {6, 2}},
        }));
    ASSERT_THROW({ dem.get_detector_coordinates({4}); }, std::invalid_argument);
}

TEST(detector_error_model, final_detector_and_coord_shift) {
    DetectorErrorModel dem(R"MODEL(
        repeat 1000 {
            repeat 2000 {
                repeat 3000 {
                    shift_detectors(0, 0, 1) 0
                }
                shift_detectors(1) 2
            }
            shift_detectors(0, 1) 0
        }
    )MODEL");
    ASSERT_EQ(
        dem.final_detector_and_coord_shift(),
        (std::pair<uint64_t, std::vector<double>>{4000000, {2000000, 1000, 6000000000}}));
}

TEST(detector_error_model, rounded) {
    DetectorErrorModel dem(R"DEM(
        error(0.01000002) D0 D1
        repeat 2 {
            error(0.123456789) D1 D2 L3
        }
        detector(0.0200000334,0.12345) D0
        shift_detectors(5.0300004,0.12345) 3
    )DEM");

    ASSERT_EQ(dem.rounded(0), DetectorErrorModel(R"DEM(
        error(0) D0 D1
        repeat 2 {
            error(0) D1 D2 L3
        }
        detector(0.0200000334,0.12345) D0
        shift_detectors(5.0300004,0.12345) 3
    )DEM"));

    ASSERT_EQ(dem.rounded(1), DetectorErrorModel(R"DEM(
        error(0) D0 D1
        repeat 2 {
            error(0.1) D1 D2 L3
        }
        detector(0.0200000334,0.12345) D0
        shift_detectors(5.0300004,0.12345) 3
    )DEM"));

    ASSERT_EQ(dem.rounded(2), DetectorErrorModel(R"DEM(
        error(0.01) D0 D1
        repeat 2 {
            error(0.12) D1 D2 L3
        }
        detector(0.0200000334,0.12345) D0
        shift_detectors(5.0300004,0.12345) 3
    )DEM"));

    ASSERT_EQ(dem.rounded(3), DetectorErrorModel(R"DEM(
        error(0.010) D0 D1
        repeat 2 {
            error(0.123) D1 D2 L3
        }
        detector(0.0200000334,0.12345) D0
        shift_detectors(5.0300004,0.12345) 3
    )DEM"));
}

TEST(detector_error_model, surface_code_coords_dont_infinite_loop) {
    CircuitGenParameters params(7, 5, "rotated_memory_x");
    params.after_clifford_depolarization = 0.01;
    params.before_measure_flip_probability = 0;
    params.after_reset_flip_probability = 0;
    params.before_round_data_depolarization = 0;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);
    std::set<uint64_t> filter;
    size_t n = dem.count_detectors();
    for (size_t k = 0; k < n; k++) {
        filter.insert(k);
    }
    auto coords1 = dem.get_detector_coordinates(filter);
    auto coords2 = circuit.get_detector_coordinates(filter);
    ASSERT_EQ(coords1.size(), coords2.size());
    ASSERT_EQ(coords1.size(), n);
    ASSERT_EQ(n, 168);
}

TEST(detector_error_model, flattened) {
    ASSERT_EQ(DetectorErrorModel().flattened(), DetectorErrorModel());

    ASSERT_EQ(
        DetectorErrorModel(R"DEM(
        error(0.125) D0 D1 L0
    )DEM")
            .flattened(),
        DetectorErrorModel(R"DEM(
        error(0.125) D0 D1 L0
    )DEM"));

    ASSERT_EQ(
        DetectorErrorModel(R"DEM(
        error(0.125) D0 D1 L0
        shift_detectors 5
    )DEM")
            .flattened(),
        DetectorErrorModel(R"DEM(
        error(0.125) D0 D1 L0
    )DEM"));

    ASSERT_EQ(
        DetectorErrorModel(R"DEM(
        shift_detectors 5
        error(0.125) D0 D1 L0
    )DEM")
            .flattened(),
        DetectorErrorModel(R"DEM(
        error(0.125) D5 D6 L0
    )DEM"));

    ASSERT_EQ(
        DetectorErrorModel(R"DEM(
        detector(10, 20) D0
        detector(10, 20, 30, 40) D1
        logical_observable L0
        shift_detectors(1, 2, 3) 5
        detector(10, 20) D0
        detector(10, 20, 30, 40) D1
        logical_observable L1
    )DEM")
            .flattened(),
        DetectorErrorModel(R"DEM(
        detector(10, 20) D0
        detector(10, 20, 30, 40) D1
        logical_observable L0
        detector(11, 22) D5
        detector(11, 22, 33, 40) D6
        logical_observable L1
    )DEM"));

    ASSERT_EQ(
        DetectorErrorModel(R"DEM(
        repeat 5 {
            error(0.125) D0
            shift_detectors(3) 2
        }
        detector(10, 20, 30, 40) D0
    )DEM")
            .flattened(),
        DetectorErrorModel(R"DEM(
        error(0.125) D0
        error(0.125) D2
        error(0.125) D4
        error(0.125) D6
        error(0.125) D8
        detector(25, 20, 30, 40) D10
    )DEM"));
}
