#include "stim/draw/diagram.h"
#include "stim/draw/json_obj.h"
#include "stim/draw/timeline_3d.h"

using namespace stim;
using namespace stim_internal;

struct V3 {
    std::array<float, 3> xyz;
    bool operator<(V3 other) const {
        if (xyz[0] != other.xyz[0]) {
            return xyz[0] < other.xyz[0];
        }
        if (xyz[1] != other.xyz[1]) {
            return xyz[1] < other.xyz[1];
        }
        return xyz[2] < other.xyz[2];
    }
    bool operator==(V3 other) const {
        return xyz == other.xyz;
    }
};

struct Triangle {
    V3 c;
    std::array<V3, 3> v;
};

template <typename T>
JsonObj make_buffer(const std::vector<T> items) {
    std::stringstream ss;
    ss << "data:application/octet-stream;base64,";
    size_t n = items.size() * sizeof(T);
    write_base64((const char *)(const void *)items.data(), n, ss);
    return std::map<std::string, JsonObj>{
        {"uri", ss.str()},
        {"byteLength", n},
    };
}

struct Material {
    std::string name;
    std::array<float, 4> base_color_factor_rgba;
    float metallic_factor;
    float roughness_factor;
    bool double_sided;

    std::pair<std::string, JsonObj> to_named_json() {
        return {name, to_json()};
    }

    JsonObj to_json() {
        return std::map<std::string, JsonObj>{
            {"name", name},
            {"pbrMetallicRoughness", std::map<std::string, JsonObj>{
                {"baseColorFactor", std::vector<JsonObj>{
                    base_color_factor_rgba[0],
                    base_color_factor_rgba[1],
                    base_color_factor_rgba[2],
                    base_color_factor_rgba[3],
                }},
                {"metallicFactor", metallic_factor},
                {"roughnessFactor", roughness_factor}}},
            {"doubleSided", double_sided},
        };
    }
};

std::string stim::circuit_diagram_timeline_3d(const Circuit &circuit) {
    constexpr size_t GL_UNSIGNED_SHORT = 5123;
    constexpr size_t GL_FLOAT = 5126;
    constexpr size_t GL_ARRAY_BUFFER = 34962;
    constexpr size_t GL_ELEMENT_ARRAY_BUFFER = 34963;
    constexpr size_t GL_TRIANGLE_STRIP = 5;
//    constexpr size_t GL_LINE_STRIP = 3;

    std::vector<Triangle> triangles;
    triangles.push_back({{1, 1, 0}, std::array<V3, 3>{{{0, 0, 0}, {1, 1, 1}, {2, 0, 0}}}});

    std::map<V3, size_t> color_to_material_index = {};
    std::map<V3, size_t> vertex_to_index = {};
    std::vector<JsonObj> materials;
    materials.push_back(Material{
        .name="reddish",
        .base_color_factor_rgba={1, 0, 0.1, 1},
        .metallic_factor=0.1,
        .roughness_factor=0.5,
        .double_sided=true,
    }.to_json());
    std::map<std::string, size_t> material_index_map;
    for (size_t k = 0; k < materials.size(); k++) {
        material_index_map.insert({materials[k].map.at("name").text, k});
    }

    std::vector<JsonObj> meshes;
    std::vector<std::vector<std::array<V3, 3>>> triangle_lists{{}};
    std::vector<float> vert_data;
    for (const auto &t : triangles) {
        for (V3 v : t.v) {
            V3 vertex_key = v;
            if (vertex_to_index.find(vertex_key) == vertex_to_index.end()) {
                auto n = vertex_to_index.size();
                vertex_to_index.insert({vertex_key, n});
                for (const auto &c : v.xyz) {
                    vert_data.push_back(c);
                }
            }
        }
        triangle_lists[0].push_back(t.v);
    }

    std::vector<JsonObj> buffer_views{
        std::map<std::string, JsonObj>{
            {"buffer", 0},
            {"byteOffset", 0},
            {"byteLength", vert_data.size() * sizeof(float)},
            {"target", GL_ARRAY_BUFFER},
        },
    };

    V3 v_min{INFINITY, INFINITY, INFINITY};
    V3 v_max{-INFINITY, -INFINITY, -INFINITY};
    for (const auto &a : triangle_lists) {
        for (const auto &b : a) {
            for (const auto &c : b) {
                for (size_t k = 0; k < 3; k++) {
                    v_min.xyz[k] = std::min(v_min.xyz[k], c.xyz[k]);
                    v_max.xyz[k] = std::max(v_max.xyz[k], c.xyz[k]);
                }
            }
        }
    }
    std::vector<JsonObj> accessors{
        std::map<std::string, JsonObj>{
            {"bufferView", 0},
            {"byteOffset", 0},
            {"componentType", GL_FLOAT},
            {"count", vert_data.size() / 3},
            {"type", "VEC3"},
            {"min", std::vector<JsonObj>{v_min.xyz[0], v_min.xyz[1], v_min.xyz[2]}},
            {"max", std::vector<JsonObj>{v_max.xyz[0], v_max.xyz[1], v_max.xyz[2]}},
        },
    };

    std::vector<uint16_t> index_data;
    size_t mesh_index = 0;
    for (const auto &tlist : triangle_lists) {
        auto index_data_offset = index_data.size();
        for (const auto &v123 : tlist) {
            auto a = vertex_to_index[v123[0]];
            auto b = vertex_to_index[v123[1]];
            auto c = vertex_to_index[v123[2]];
            index_data.push_back(a);
            index_data.push_back(b);
            index_data.push_back(c);
        }

        meshes.push_back(std::map<std::string, JsonObj>{
            {"primitives",
             std::vector<JsonObj>{{std::map<std::string, JsonObj>{
                 {"attributes", {std::map<std::string, JsonObj>{{"POSITION", 0}}}},
                 {"indices", mesh_index + 1},
                 {"material", material_index_map.at("reddish")},
                 {"mode", GL_TRIANGLE_STRIP},
             }}}}});

        buffer_views.push_back(std::map<std::string, JsonObj>{
            {"buffer", 1},
            {"byteOffset", index_data_offset},
            {"byteLength", index_data.size() * sizeof(float) * 3 - index_data_offset},
            {"target", GL_ELEMENT_ARRAY_BUFFER},
        });
        accessors.push_back(std::map<std::string, JsonObj>{
            {"bufferView", mesh_index + 1},
            {"byteOffset", 0},
            {"componentType", GL_UNSIGNED_SHORT},
            {"count", tlist.size() * 3},
            {"type", "SCALAR"},
            {"max", std::vector<JsonObj>{vertex_to_index.size() - 1}},
            {"min", std::vector<JsonObj>{0}},
        });
        mesh_index++;
    }

    std::vector<JsonObj> scene_nodes;
    for (size_t k = 0; k < triangle_lists.size(); k++) {
        scene_nodes.push_back(k);
    }
    std::vector<JsonObj> nodes;
    for (size_t k = 0; k < triangle_lists.size(); k++) {
        nodes.push_back(std::map<std::string, JsonObj>{
            {"mesh", k},
        });
    }

    JsonObj result(std::map<std::string, JsonObj>{
        {"scene", 0},
        {"scenes", std::vector<JsonObj>{
            std::map<std::string, JsonObj>{
                {"nodes", std::move(scene_nodes)},
            },
        }},
        {"asset", std::map<std::string, JsonObj>{
            {"version", "2.0"},
        }},
        {"nodes", std::move(nodes)},
        {"meshes", std::move(meshes)},
        {"buffers", std::vector<JsonObj>{
            make_buffer(vert_data),
            make_buffer(index_data),
        }},
        {"bufferViews", std::move(buffer_views)},
        {"accessors", std::move(accessors)},
        {"materials", std::move(materials)},
    });
    return result.str();
}
