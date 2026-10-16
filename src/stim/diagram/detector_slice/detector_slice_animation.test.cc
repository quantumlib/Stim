#include "stim/diagram/detector_slice/detector_slice_animation.h"

#include <sstream>

#include "gtest/gtest.h"

#include "stim/diagram/diagram_util.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(detector_slice_animation, frames_match_existing_renderer) {
    Circuit circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(0, 1) 3
        QUBIT_COORDS(1, 1) 4
        QUBIT_COORDS(2, 1) 5
        RX 0 1 3 4
        R 2 5
        TICK
        MPP X0 X1*Z2 X3*X4*Z5
        DETECTOR(0) rec[-3]
        DETECTOR(1) rec[-2]
        DETECTOR(2) rec[-1]
    )CIRCUIT");
    CoordFilter all;
    auto frames = make_detector_slice_animation_frames(circuit, 0, 2, {&all});

    ASSERT_EQ(frames.size(), 2);
    for (const auto &frame : frames) {
        std::stringstream expected;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit, expected, frame.tick, 1, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&all});
        EXPECT_EQ(frame.svg, expected.str());
    }

    const auto &regions = frames[0].detector_regions;
    ASSERT_EQ(regions.size(), 3);
    EXPECT_EQ(regions.at(0).path.kind, DetectorSliceSvgPathKind::CIRCLE);
    EXPECT_EQ(regions.at(1).path.kind, DetectorSliceSvgPathKind::PATH);
    EXPECT_EQ(regions.at(2).path.kind, DetectorSliceSvgPathKind::PATH);
    EXPECT_EQ(regions.at(0).style.fill, X_RED);
    EXPECT_EQ(regions.at(1).style.fill, BG_GREY);
    EXPECT_EQ(regions.at(2).style.fill, BG_GREY);
    EXPECT_TRUE(regions.at(0).style.gradients.empty());
    EXPECT_EQ(regions.at(1).style.gradients.size(), 2);
    EXPECT_EQ(regions.at(2).style.gradients.size(), 3);
    EXPECT_FLOAT_EQ(regions.at(2).style.fill_opacity, 0.75f);
}

TEST(detector_slice_animation, transitions) {
    DetectorSliceSvgPath circle{DetectorSliceSvgPathKind::CIRCLE, {0, 0}, 1, {}};
    DetectorSliceSvgPath death_circle{DetectorSliceSvgPathKind::CIRCLE, {3, 4}, 1, {}};
    DetectorSliceSvgPath birth_circle{DetectorSliceSvgPathKind::CIRCLE, {-2, 5}, 1, {}};
    DetectorSliceSvgPath lens{
        DetectorSliceSvgPathKind::PATH,
        {-1, 0},
        0,
        {
            {false, {-1, 1}, {1, 1}, {1, 0}},
            {false, {1, -1}, {-1, -1}, {-1, 0}},
        },
    };
    DetectorSliceSvgPath square{
        DetectorSliceSvgPathKind::PATH,
        {0, 0},
        0,
        {
            {true, {}, {}, {1, 0}},
            {true, {}, {}, {1, 1}},
            {true, {}, {}, {0, 1}},
            {true, {}, {}, {0, 0}},
        },
    };
    DetectorSliceSvgPath reversed_square{
        DetectorSliceSvgPathKind::PATH,
        {1, 1},
        0,
        {
            {true, {}, {}, {1, 0}},
            {true, {}, {}, {0, 0}},
            {true, {}, {}, {0, 1}},
            {true, {}, {}, {1, 1}},
        },
    };
    DetectorSliceSvgStyle x_style{X_RED, 1, {}};
    DetectorSliceSvgStyle z_style{Z_BLUE, 1, {}};
    DetectorSliceAnimationFrame source{
        0,
        {},
        {
            {0, {circle, x_style, 0}},
            {1, {circle, x_style, 1}},
            {2, {death_circle, x_style, 2}},
            {4, {circle, x_style, 4}},
            {5, {square, x_style, 5}},
        },
    };
    DetectorSliceAnimationFrame target{
        1,
        {},
        {
            {0, {circle, x_style, 0}},
            {1, {lens, x_style, 1}},
            {3, {birth_circle, x_style, 3}},
            {4, {circle, z_style, 4}},
            {5, {reversed_square, x_style, 5}},
        },
    };

    auto transitions = make_detector_slice_animation_transitions(source, target);
    ASSERT_EQ(transitions.size(), 5);

    EXPECT_EQ(transitions[0].detector_id, 1);
    EXPECT_TRUE(transitions[0].has_source);
    EXPECT_TRUE(transitions[0].has_target);
    EXPECT_TRUE(transitions[0].geometry_changed);
    EXPECT_FALSE(transitions[0].style_changed);
    EXPECT_EQ(transitions[0].source_points.size(), transitions[0].target_points.size());
    ASSERT_EQ(transitions[0].source_points.size(), 64);
    EXPECT_EQ(transitions[0].source_points[0], (Coord<2>{1, 0}));

    EXPECT_EQ(transitions[1].detector_id, 2);
    EXPECT_TRUE(transitions[1].has_source);
    EXPECT_FALSE(transitions[1].has_target);
    ASSERT_FALSE(transitions[1].target_points.empty());
    for (const auto &point : transitions[1].target_points) {
        EXPECT_NEAR(point.xyz[0], 3, 1e-5);
        EXPECT_NEAR(point.xyz[1], 4, 1e-5);
    }

    EXPECT_EQ(transitions[2].detector_id, 3);
    EXPECT_FALSE(transitions[2].has_source);
    EXPECT_TRUE(transitions[2].has_target);
    ASSERT_FALSE(transitions[2].source_points.empty());
    for (const auto &point : transitions[2].source_points) {
        EXPECT_NEAR(point.xyz[0], -2, 1e-5);
        EXPECT_NEAR(point.xyz[1], 5, 1e-5);
    }

    EXPECT_EQ(transitions[3].detector_id, 4);
    EXPECT_TRUE(transitions[3].has_source);
    EXPECT_TRUE(transitions[3].has_target);
    EXPECT_FALSE(transitions[3].geometry_changed);
    EXPECT_TRUE(transitions[3].style_changed);
    EXPECT_TRUE(transitions[3].source_points.empty());
    EXPECT_TRUE(transitions[3].target_points.empty());

    const auto &transition = transitions[4];
    EXPECT_EQ(transition.detector_id, 5);
    EXPECT_TRUE(transition.geometry_changed);
    EXPECT_FALSE(transition.style_changed);
    ASSERT_EQ(transition.source_points.size(), 64);
    ASSERT_EQ(transition.target_points.size(), 64);
    for (size_t k = 0; k < 64; k++) {
        EXPECT_NEAR((transition.source_points[(k + 1) % 64] - transition.source_points[k]).norm(), 0.0625, 1e-6);
        EXPECT_NEAR(transition.source_points[k].xyz[0], transition.target_points[k].xyz[0], 1e-6);
        EXPECT_NEAR(transition.source_points[k].xyz[1], transition.target_points[k].xyz[1], 1e-6);
    }
}
