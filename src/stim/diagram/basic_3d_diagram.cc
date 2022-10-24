#include "stim/diagram/basic_3d_diagram.h"

#include "gate_data_3d.h"

using namespace stim;
using namespace stim_draw_internal;

GltfScene Basic3dDiagram::to_gltf_scene() const {
    GltfScene scene{{"everything"}, {}};

    auto black_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"black"},
        {0, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });

    auto red_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"red"},
        {1, 0, 0, 1},
        1,
        1,
        true,
        nullptr,
    });

    auto blue_material = std::shared_ptr<GltfMaterial>(new GltfMaterial{
        {"blue"},
        {0, 0, 1, 1},
        1,
        1,
        true,
        nullptr,
    });

    auto buf_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_scattered_lines"},
        line_data,
    });

    auto buf_red_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_red_scattered_lines"},
        red_line_data,
    });

    auto buf_blue_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_blue_scattered_lines"},
        blue_line_data,
    });

    auto gate_data = make_gate_primitives();
    for (const auto &g : elements) {
        auto p = gate_data.find(g.gate_piece);
        if (p == gate_data.end()) {
            throw std::invalid_argument("Unknown gate piece: " + g.gate_piece);
        }
        scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
            {""},
            p->second,
            g.center,
        }));
    }

    if (!buf_scattered_lines->vertices.empty()) {
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
    }

    if (!buf_red_scattered_lines->vertices.empty()) {
        scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
            {"node_red_scattered_lines"},
            std::shared_ptr<GltfMesh>(new GltfMesh{
                {"mesh_scattered_lines"},
                {
                    std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                        {"primitive_red_scattered_lines"},
                        GL_LINES,
                        buf_red_scattered_lines,
                        nullptr,
                        red_material,
                    }),
                },
            }),
            {0, 0, 0},
        }));
    }

    if (!buf_blue_scattered_lines->vertices.empty()) {
        scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
            {"node_red_scattered_lines"},
            std::shared_ptr<GltfMesh>(new GltfMesh{
                {"mesh_scattered_lines"},
                {
                    std::shared_ptr<GltfPrimitive>(new GltfPrimitive{
                        {"primitive_blue_scattered_lines"},
                        GL_LINES,
                        buf_blue_scattered_lines,
                        nullptr,
                        blue_material,
                    }),
                },
            }),
            {0, 0, 0},
        }));
    }

    return scene;
}
