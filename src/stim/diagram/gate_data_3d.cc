#include "stim/diagram/gate_data_3d.h"

#include "stim/diagram/gate_data_3d_texture_data.h"

using namespace stim;
using namespace stim_draw_internal;

constexpr float CONTROL_RADIUS = 0.4f;

std::shared_ptr<GltfBuffer<2>> texture_coords_for_showing_on_spacelike_faces_of_cube(
    std::string_view name, size_t tex_tile_x, size_t tex_tile_y, bool actually_just_square) {
    constexpr size_t diam = 16;
    float d = (float)1.0 / diam;
    float dx = d * tex_tile_x;
    float dy = d * tex_tile_y;
    Coord<2> v00{dx + 0, dy + 0};
    Coord<2> v01{dx + 0, dy + d};
    Coord<2> v10{dx + d, dy + 0};
    Coord<2> v11{dx + d, dy + d};
    if (actually_just_square) {
        return std::shared_ptr<GltfBuffer<2>>(new GltfBuffer<2>(
            {{std::string(name)},
             {
                 v10,
                 v00,
                 v11,
                 v00,
                 v01,
                 v11,
                 v11,
                 v10,
                 v01,
                 v01,
                 v10,
                 v00,
             }}));
    }

    return std::shared_ptr<GltfBuffer<2>>(new GltfBuffer<2>(
        {{std::string(name)},
         {
             v00, v01, v10, v01, v11, v10, v00, v00, v00, v00, v00, v00, v10, v00, v11, v00, v01, v11,
             v01, v11, v00, v00, v11, v10, v00, v00, v00, v00, v00, v00, v11, v10, v01, v01, v10, v00,
         }}));
}

std::shared_ptr<GltfPrimitive> cube_gate(
    std::string_view gate_canonical_name,
    size_t tex_tile_x,
    size_t tex_tile_y,
    std::shared_ptr<GltfBuffer<3>> cube_position_buffer,
    std::shared_ptr<GltfMaterial> material,
    bool actually_just_square) {
    return std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {"primitive_gate_" + std::string(gate_canonical_name)},
        GL_TRIANGLES,
        cube_position_buffer,
        texture_coords_for_showing_on_spacelike_faces_of_cube(
            "tex_coords_gate_" + std::string(gate_canonical_name), tex_tile_x, tex_tile_y, actually_just_square),
        material,
    });
}

std::shared_ptr<GltfBuffer<3>> make_cube_triangle_list(bool actually_just_square) {
    Coord<3> v000{-0.5f, +0.5f, +0.5f};
    Coord<3> v001{-0.5f, +0.5f, -0.5f};
    Coord<3> v010{-0.5f, -0.5f, +0.5f};
    Coord<3> v011{-0.5f, -0.5f, -0.5f};
    Coord<3> v100{+0.5f, +0.5f, +0.5f};
    Coord<3> v101{+0.5f, +0.5f, -0.5f};
    Coord<3> v110{+0.5f, -0.5f, +0.5f};
    Coord<3> v111{+0.5f, -0.5f, -0.5f};
    if (actually_just_square) {
        v000.xyz[0] = 0;
        v001.xyz[0] = 0;
        v010.xyz[0] = 0;
        v011.xyz[0] = 0;
        return std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
            {"cube"},
            {
                v000,
                v001,
                v010,
                v001,
                v011,
                v010,

                v011,
                v001,
                v010,
                v010,
                v001,
                v000,
            }});
    }
    return std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"cube"},
        {
            v000, v010, v100, v010, v110, v100,

            v000, v100, v001, v001, v100, v101,

            v000, v001, v010, v001, v011, v010,

            v111, v011, v101, v101, v011, v001,

            v111, v110, v011, v110, v010, v011,

            v111, v101, v110, v110, v101, v100,
        }});
}

std::shared_ptr<GltfBuffer<3>> make_circle_loop(size_t n, float r, bool repeat_boundary) {
    std::vector<Coord<3>> vertices;
    vertices.push_back({0, r, 0});
    for (size_t k = 1; k < n; k++) {
        float t = k * 3.14159265359f * 2 / n;
        vertices.push_back({0, cosf(t) * r, sinf(t) * r});
    }
    if (repeat_boundary) {
        vertices.push_back({0, r, 0});
    }
    return std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"circle_loop"},
        std::move(vertices),
    });
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_x_control_mesh() {
    auto line_cross = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"control_x_line_cross"},
        {{0, -CONTROL_RADIUS, 0}, {0, +CONTROL_RADIUS, 0}, {0, 0, -CONTROL_RADIUS}, {0, 0, +CONTROL_RADIUS}},
    });
    auto circle = make_circle_loop(16, CONTROL_RADIUS, true);
    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto white_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"white"},
        {1, 1, 1, 1},
        0.4,
        0.5,
        true,
        nullptr,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_X_CONTROL"},
        {
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_circle_interior"},
                GL_TRIANGLE_FAN,
                circle,
                nullptr,
                white_material,
            }),
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_circle_perimeter"},
                GL_LINE_STRIP,
                circle,
                nullptr,
                black_material,
            }),
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_line_cross"},
                GL_LINES,
                line_cross,
                nullptr,
                black_material,
            }),
        },
    });
    return {"X_CONTROL", mesh};
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_xswap_control_mesh() {
    float h = CONTROL_RADIUS * sqrtf(2) * 0.8f;
    auto line_cross = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"control_xswap_line_cross"},
        {{0, -h, -h}, {0, +h, +h}, {0, -h, +h}, {0, +h, -h}},
    });
    auto circle = make_circle_loop(16, CONTROL_RADIUS, true);
    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto white_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"white"},
        {1, 1, 1, 1},
        0.4,
        0.5,
        true,
        nullptr,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_XSWAP_CONTROL"},
        {
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_circle_interior"},
                GL_TRIANGLE_FAN,
                circle,
                nullptr,
                white_material,
            }),
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_circle_perimeter"},
                GL_LINE_STRIP,
                circle,
                nullptr,
                black_material,
            }),
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_line_cross"},
                GL_LINES,
                line_cross,
                nullptr,
                black_material,
            }),
        },
    });
    return {"XSWAP", mesh};
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_zswap_control_mesh() {
    float h = CONTROL_RADIUS * sqrtf(2) * 0.8f;
    auto line_cross = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"control_zswap_line_cross"},
        {{0, -h, -h}, {0, +h, +h}, {0, -h, +h}, {0, +h, -h}},
    });
    auto circle = make_circle_loop(16, CONTROL_RADIUS, true);
    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto white_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"white"},
        {1, 1, 1, 1},
        0.4,
        0.5,
        true,
        nullptr,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_XSWAP_CONTROL"},
        {
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_circle_interior"},
                GL_TRIANGLE_FAN,
                circle,
                nullptr,
                black_material,
            }),
            std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                {"primitive_line_cross"},
                GL_LINES,
                line_cross,
                nullptr,
                white_material,
            }),
        },
    });
    return {"ZSWAP", mesh};
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_y_control_mesh() {
    auto gray_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"gray"},
        {0.5, 0.5, 0.5, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto tri_buf = make_circle_loop(3, CONTROL_RADIUS, false);
    auto triangle_perimeter = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {"primitive_triangle_perimeter"},
        GL_LINE_LOOP,
        tri_buf,
        nullptr,
        black_material,
    });
    auto triangle_interior = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {"primitive_triangle_interior"},
        GL_TRIANGLES,
        tri_buf,
        nullptr,
        gray_material,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_control_Y"},
        {
            triangle_perimeter,
            triangle_interior,
        },
    });
    return {"Y_CONTROL", mesh};
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_z_control_mesh() {
    auto circle = make_circle_loop(16, CONTROL_RADIUS, true);
    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto disc_interior = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {"primitive_circle_interior"},
        GL_TRIANGLE_FAN,
        circle,
        nullptr,
        black_material,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_Z_CONTROL"},
        {
            disc_interior,
        },
    });
    return {"Z_CONTROL", mesh};
}

std::pair<std::string_view, std::shared_ptr<GltfMesh>> make_detector_mesh(bool excited) {
    auto circle = make_circle_loop(8, CONTROL_RADIUS, true);
    auto circle2 = make_circle_loop(8, CONTROL_RADIUS, true);
    auto circle3 = make_circle_loop(8, CONTROL_RADIUS, true);
    for (auto &e : circle2->vertices) {
        std::swap(e.xyz[1], e.xyz[2]);
        std::swap(e.xyz[0], e.xyz[1]);
    }
    for (auto &e : circle3->vertices) {
        std::swap(e.xyz[0], e.xyz[1]);
        std::swap(e.xyz[1], e.xyz[2]);
    }
    auto material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {excited ? "det_red" : "det_black"},
        {excited ? 1.0f : 0.0f, excited ? 0.5f : 0.0f, excited ? 0.5f : 0.0f, 1},
        1,
        1,
        true,
        nullptr,
    });
    auto disc_interior = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {excited ? "excited_detector_primitive_circle_interior" : "detector_primitive_circle_interior"},
        GL_TRIANGLE_FAN,
        circle,
        nullptr,
        material,
    });
    auto disc_interior2 = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {excited ? "excited_detector_primitive_circle_interior_2" : "detector_primitive_circle_interior_2"},
        GL_TRIANGLE_FAN,
        circle2,
        nullptr,
        material,
    });
    auto disc_interior3 = std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {excited ? "excited_detector_primitive_circle_interior_3" : "detector_primitive_circle_interior_3"},
        GL_TRIANGLE_FAN,
        circle3,
        nullptr,
        material,
    });
    auto mesh = std::shared_ptr<GltfMesh>(new GltfMesh{
        {excited ? "mesh_EXCITED_DETECTOR" : "mesh_DETECTOR"},
        {
            disc_interior,
            disc_interior2,
            disc_interior3,
        },
    });
    return {excited ? "EXCITED_DETECTOR" : "DETECTOR", mesh};
}

std::map<std::string_view, std::shared_ptr<GltfMesh>> stim_draw_internal::make_gate_primitives() {
    bool actually_square = true;
    auto cube = make_cube_triangle_list(actually_square);
    auto image = std::shared_ptr<GltfImage>(new GltfImage{
        {"gates_image"},
        make_gate_3d_texture_data_uri(),
    });
    auto sampler = std::shared_ptr<GltfSampler>(new GltfSampler{
        {"gates_sampler"},
        GL_NEAREST,
        GL_NEAREST,
        GL_CLAMP_TO_EDGE,
        GL_CLAMP_TO_EDGE,
    });
    auto texture = std::shared_ptr<GltfTexture>(new GltfTexture{
        {"gates_texture"},
        sampler,
        image,
    });
    auto material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"gates_material"},
        {1, 1, 1, 1},
        0.4,
        0.5,
        false,
        texture,
    });

    auto f = [&](std::string_view s, size_t x, size_t y) -> std::pair<std::string_view, std::shared_ptr<GltfMesh>> {
        return {s, GltfMesh::from_singleton_primitive(cube_gate(s, x, y, cube, material, actually_square))};
    };

    return std::map<std::string_view, std::shared_ptr<GltfMesh>>{
        f("X", 0, 6),
        f("Y", 0, 7),
        f("Z", 0, 8),

        f("H_YZ", 1, 6),
        f("H", 1, 7),
        f("H_XY", 1, 8),

        f("SQRT_X", 2, 6),
        f("SQRT_Y", 2, 7),
        f("S", 2, 8),

        f("SQRT_X_DAG", 3, 6),
        f("SQRT_Y_DAG", 3, 7),
        f("S_DAG", 3, 8),

        f("MX", 4, 6),
        f("MY", 4, 7),
        f("M", 4, 8),

        f("RX", 5, 6),
        f("RY", 5, 7),
        f("R", 5, 8),

        f("MRX", 6, 6),
        f("MRY", 6, 7),
        f("MR", 6, 8),

        f("X_ERROR", 7, 6),
        f("Y_ERROR", 7, 7),
        f("Z_ERROR", 7, 8),

        f("E:X", 8, 6),
        f("E:Y", 8, 7),
        f("E:Z", 8, 8),

        f("ELSE_CORRELATED_ERROR:X", 9, 6),
        f("ELSE_CORRELATED_ERROR:Y", 9, 7),
        f("ELSE_CORRELATED_ERROR:Z", 9, 8),

        f("MPP:X", 10, 6),
        f("MPP:Y", 10, 7),
        f("MPP:Z", 10, 8),

        f("SQRT_XX", 11, 6),
        f("SQRT_YY", 11, 7),
        f("SQRT_ZZ", 11, 8),

        f("SQRT_XX_DAG", 12, 6),
        f("SQRT_YY_DAG", 12, 7),
        f("SQRT_ZZ_DAG", 12, 8),

        f("X:REC", 13, 6),
        f("Y:REC", 13, 7),
        f("Z:REC", 13, 8),

        f("X:SWEEP", 14, 6),
        f("Y:SWEEP", 14, 7),
        f("Z:SWEEP", 14, 8),

        f("I", 0, 6),
        f("C_XYZ", 1, 9),
        f("C_NXYZ", 6, 10),
        f("C_XNYZ", 7, 10),
        f("C_XYNZ", 8, 10),
        f("C_ZYX", 2, 9),
        f("C_NZYX", 9, 10),
        f("C_ZNYX", 10, 10),
        f("C_ZYNX", 11, 10),
        f("H_NXY", 12, 10),
        f("H_NXZ", 13, 10),
        f("H_NYZ", 14, 10),
        f("II", 15, 10),
        f("II_ERROR", 15, 11),
        f("I_ERROR", 15, 8),
        f("DEPOLARIZE1", 3, 9),
        f("DEPOLARIZE2", 4, 9),
        f("ISWAP", 5, 9),
        f("ISWAP_DAG", 6, 9),
        f("SWAP", 7, 9),
        f("PAULI_CHANNEL_1", 8, 9),
        f("PAULI_CHANNEL_2", 9, 9),
        f("MXX", 10, 9),
        f("MYY", 11, 9),
        f("MZZ", 12, 9),
        f("MPAD", 13, 9),
        f("HERALDED_ERASE", 14, 9),
        f("HERALDED_PAULI_CHANNEL_1", 15, 9),

        f("SPP:X", 0, 10),
        f("SPP:Y", 1, 10),
        f("SPP:Z", 2, 10),
        f("SPP_DAG:X", 3, 10),
        f("SPP_DAG:Y", 4, 10),
        f("SPP_DAG:Z", 5, 10),

        make_x_control_mesh(),
        make_y_control_mesh(),
        make_z_control_mesh(),
        make_xswap_control_mesh(),
        make_zswap_control_mesh(),
        make_detector_mesh(false),
        make_detector_mesh(true),
    };
}
