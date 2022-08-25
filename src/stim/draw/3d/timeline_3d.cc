#include "stim/draw/3d/timeline_3d.h"
#include "stim/draw/3d/gate_data_3d.h"
#include "stim/draw/3d/diagram_3d.h"

using namespace stim;
using namespace stim_draw_internal;

std::string stim::circuit_diagram_timeline_3d(const Circuit &circuit) {
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

    auto diagram = Diagram3D::from_circuit(circuit);
    auto buf_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_scattered_lines"},
        std::move(diagram.line_data),
    });
    auto buf_red_scattered_lines = std::shared_ptr<GltfBuffer<3>>(new GltfBuffer<3>{
        {"buf_red_scattered_lines"},
        std::move(diagram.red_line_data),
    });

    auto gate_data = make_gate_primitives();
    for (const auto &g : diagram.gates) {
        auto p = gate_data.find(g.gate_piece);
        if (p != gate_data.end()) {
            scene.nodes.push_back(std::shared_ptr<GltfNode>(new GltfNode{
                {""},
                p->second,
                g.center,
            }));
        }
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

    return scene.to_json().str(false);
}
