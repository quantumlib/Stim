#include "stim/draw/3d/gate_data_3d.h"
#include "stim/draw/3d/gate_data_3d_texture_data.h"

using namespace stim;
using namespace stim_draw_internal;

std::shared_ptr<GltfBuffer<2>> texture_coords_for_showing_on_spacelike_faces_of_cube(const std::string &name, size_t tex_tile_x, size_t tex_tile_y) {
    constexpr size_t diam = 16;
    float d = (float)1.0 / diam;
    float dx = d * tex_tile_x;
    float dy = d * tex_tile_y;
    Coord<2> v00{dx + 0, dy + 0};
    Coord<2> v01{dx + 0, dy + d};
    Coord<2> v10{dx + d, dy + 0};
    Coord<2> v11{dx + d, dy + d};

    return std::shared_ptr<GltfBuffer<2>>(new GltfBuffer<2>({{name}, {
        v00, v01, v10, v01, v11, v10,
        v00, v00, v00, v00, v00, v00,
        v10, v00, v11, v00, v01, v11,
        v01, v11, v00, v00, v11, v10,
        v00, v00, v00, v00, v00, v00,
        v11, v10, v01, v01, v10, v00,
    }}));
}

std::shared_ptr<GltfPrimitive> cube_gate(
        const std::string &gate_canonical_name,
        size_t tex_tile_x,
        size_t tex_tile_y,
        std::shared_ptr<GltfBuffer<3>> cube_position_buffer,
        std::shared_ptr<GltfMaterial> material) {
    return std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
        {"primitive_gate_" + gate_canonical_name},
        GL_TRIANGLES,
        cube_position_buffer,
        texture_coords_for_showing_on_spacelike_faces_of_cube(
            "tex_coords_gate_" + gate_canonical_name,
            tex_tile_x,
            tex_tile_y),
        material,
    });
}

std::shared_ptr<GltfBuffer<3>> make_cube_triangle_list() {
    Coord<3> v000{-0.5f, +0.5f, +0.5f};
    Coord<3> v001{-0.5f, +0.5f, -0.5f};
    Coord<3> v010{-0.5f, -0.5f, +0.5f};
    Coord<3> v011{-0.5f, -0.5f, -0.5f};
    Coord<3> v100{+0.5f, +0.5f, +0.5f};
    Coord<3> v101{+0.5f, +0.5f, -0.5f};
    Coord<3> v110{+0.5f, -0.5f, +0.5f};
    Coord<3> v111{+0.5f, -0.5f, -0.5f};
    return std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{{"cube"}, {
        v000, v010, v100,
        v010, v110, v100,
        v000, v100, v001,
        v001, v100, v101,

        v000, v001, v010,
        v001, v011, v010,

        v111, v011, v101,
        v101, v011, v001,

        v111, v110, v011,
        v110, v010, v011,

        v111, v101, v110,
        v110, v101, v100,
    }});
}

std::map<std::string, std::shared_ptr<GltfPrimitive>> stim_draw_internal::make_gate_primitives() {
    auto cube = make_cube_triangle_list();
    auto image = std::shared_ptr<GltfImage>(new GltfImage{
        {"gates_image"},
        GATE_DATA_3D_TEXTURE_DATA_URI,
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

    auto f = [&](const char *s, size_t x, size_t y) -> std::pair<std::string, std::shared_ptr<GltfPrimitive>>{
        return {s, cube_gate(s, x, y, cube, material)};
    };

    return std::map<std::string, std::shared_ptr<GltfPrimitive>>{
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

        f("X_ERR", 7, 6),
        f("Y_ERR", 7, 7),
        f("Z_ERR", 7, 8),

        f("E:X", 8, 6),
        f("E:Y", 8, 7),
        f("E:Z", 8, 8),

        f("ELSE_CORRELATED_ERROR:X", 9, 6),
        f("ELSE_CORRELATED_ERROR:Y", 9, 7),
        f("ELSE_CORRELATED_ERROR:Z", 9, 8),

        f("C_XYZ", 0, 9),
        f("C_ZYX", 1, 9),
        f("DEPOLARIZE1", 2, 9),
        f("DEPOLARIZE2", 3, 9),
    };
};
