#include "stim/draw/diagram.h"
#include "stim/draw/3d/json_obj.h"
#include "stim/draw/3d/timeline_3d.h"
#include "stim/draw/3d/gate_data_3d.h"

using namespace stim;
using namespace stim_draw_internal;

//template <typename T>
//JsonObj make_buffer(const std::vector<T> items) {
//    std::stringstream ss;
//    ss << "data:application/octet-stream;base64,";
//    size_t n = items.size() * sizeof(T);
//    write_base64((const char *)(const void *)items.data(), n, ss);
//    return std::map<std::string, JsonObj>{
//        {"uri", ss.str()},
//        {"byteLength", n},
//    };
//}


//std::vector<V<3>> circle_line_loop(size_t n) {
//    std::vector<V<3>> result;
//    result.push_back({0.5f, 0, 0});
//    for (size_t k = 1; k < n; k++) {
//        float t = k*M_PI*2/n;
//        result.push_back({cosf(t) * 0.5f, 0, sinf(t) * 0.5f});
//    }
//    result.push_back({0.5f, 0, 0});
//    return result;
//}

std::string stim::circuit_diagram_timeline_3d(const Circuit &circuit) {
    GltfScene scene{{"everything"}, {}};

    auto white_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"white"},
        {1, 1, 1, 1},
        0.4,
        0.5,
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
    auto gray_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"gray"},
        {0.5, 0.5, 0.5, 1},
        1,
        1,
        true,
        nullptr,
    });

    auto tri_buf = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"triangle"},
        {{-0.5f, 0, -0.5f}, {+0.5f, 0, -0.5f}, {0, 0, +0.5f}},
    });
//    std::vector<Buf<3>> position_buffers{
//        {"disk_triangle_fan", circle_line_loop(16)},
//        {"circle_line_strip", circle_line_loop(16)},
//        {"triangle_perimeter_line_loop", std::vector<V<3>>{{-0.5f, 0, -0.5f}, {+0.5f, 0, -0.5f}, {0, 0, +0.5f}}},
//        {"triangle_interior_triangle_list", std::vector<V<3>>{{-0.5f, 0, -0.5f}, {+0.5f, 0, -0.5f}, {0, 0, +0.5f}}},
//        {"cube_triangle_list", cube_triangle_list()},
//        {"cross_lines", {{-0.5f, 0, 0}, {+0.5f, 0, 0}, {0, 0, -0.5f}, {0, 0, +0.5f}}},
//        {"abs_lines", {}},
//    };

//    primitives.push_back({.name="disk", .positions_buf_name="disk_triangle_fan", .tex_coords={}, .element_type=GL_TRIANGLE_FAN});
//    primitives.push_back({.name="circle", .positions_buf_name="circle_line_strip", .tex_coords={}, .element_type=GL_LINE_STRIP});

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
//    primitives.push_back({.name="cross", .positions_buf_name="cross_lines", .tex_coords={}, .element_type=GL_LINES});
//    primitives.push_back({.name="abs_lines", .positions_buf_name="abs_lines", .tex_coords={}, .element_type=GL_LINES});

//    meshes.push_back(Mesh{
//        "control_X",
//        {"disk", "circle", "cross"},
//        {"white", "black", "black"}
//    });
//    meshes.push_back(Mesh{
//        "control_Y",
//        {"triangle_perimeter", "triangle_interior"},
//        {"black", "gray"}
//    });
//    meshes.push_back(Mesh{
//        "control_Z",
//        {"disk"},
//        {"black"}
//    });
//    meshes.push_back(Mesh{
//        "abs_lines",
//        {"abs_lines"},
//        {"black"}
//    });

    Diagram d = to_diagram(circuit);
    auto buf_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_scattered_lines"},
        {},
    });
    for (const auto &line : d.lines) {
        buf_scattered_lines->vertices.push_back({(float)line.p1.x, (float)line.p1.y, (float)line.p1.z});
        buf_scattered_lines->vertices.push_back({(float)line.p2.x, (float)line.p2.y, (float)line.p2.z});
    }

    scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
        {"node_control_Y"},
        std::shared_ptr<GltfMesh>(new GltfMesh{
            {"mesh_control_Y"},
            {
                triangle_perimeter,
                triangle_interior,
            },
        }),
        {0, 0, 0},
    }));
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("control_Y")},
//        {"translation", std::vector<JsonObj>{
//            0,
//            0,
//            0,
//        }},
//    });
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("control_Z")},
//        {"translation", std::vector<JsonObj>{
//            2,
//            0,
//            0,
//        }},
//    });
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("gate_H")},
//        {"translation", std::vector<JsonObj>{
//            0,
//            2,
//            0,
//        }},
//    });
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("gate_H_XY")},
//        {"translation", std::vector<JsonObj>{
//            3,
//            2,
//            0,
//        }},
//    });
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("gate_S")},
//        {"translation", std::vector<JsonObj>{
//            5,
//            2,
//            0,
//        }},
//    });
//    nodes_json.push_back(std::map<std::string, JsonObj>{
//        {"mesh", mesh_index_map.at("gate_SQRT_X")},
//        {"translation", std::vector<JsonObj>{
//            7,
//            2,
//            0,
//        }},
//    });
    scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
        {"node_scattered_lines"},
        std::shared_ptr<GltfMesh>(new GltfMesh{
            {"mesh_scattered_lines"},
            {
                std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                    {"primitive_scattered_lines"},
                    GL_LINES,
                    buf_scattered_lines,
                    nullptr,
                    black_material,
                }),
            },
        }),
        {0, 0, 0},
    }));

    return scene.to_json().str(true);
}
