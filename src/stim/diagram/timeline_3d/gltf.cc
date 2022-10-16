#include <iostream>

#include "stim/diagram/timeline_3d/gltf.h"
#include "stim/diagram/timeline_3d/diagram_3d.h"
#include "stim/diagram/timeline_3d/gate_data_3d.h"

using namespace stim;
using namespace stim_draw_internal;

void GltfScene::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "scenes",
        [&]() {
            return _to_json_local();
        },
        (uintptr_t)this);
    for (auto &node : nodes) {
        node->visit(callback);
    }
}

JsonObj GltfScene::_to_json_local() const {
    std::vector<JsonObj> scene_nodes_json;
    for (const auto &n : nodes) {
        scene_nodes_json.push_back(n->id.index);
    }
    return std::map<std::string, JsonObj>{
        {"nodes", std::move(scene_nodes_json)},
    };
}

JsonObj GltfScene::to_json() {
    // Clear indices.
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        item_id.index = SIZE_MAX;
    });

    // Re-index.
    std::map<std::string, size_t> counts;
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        auto &c = counts[type];
        if (item_id.index == SIZE_MAX || item_id.index == c) {
            item_id.index = c;
            c++;
        } else if (item_id.index > c) {
            throw std::invalid_argument("out of order");
        }
    });

    std::map<std::string, JsonObj> result{
        {"scene", 0},
        {"asset", std::map<std::string, JsonObj>{
             {"version", "2.0"},
        }},
    };
    for (const auto &r : counts) {
        result.insert({r.first, std::vector<JsonObj>{}});
    }
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        auto &list = result.at(type).arr;
        if (item_id.index == list.size()) {
            list.push_back(to_json());
        }
    });

    return result;
}

GltfScene GltfScene::from_circuit(const stim::Circuit &circuit){
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

    return scene;
}
