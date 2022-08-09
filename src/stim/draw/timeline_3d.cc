#include "stim/draw/diagram.h"
#include "stim/draw/json_obj.h"
#include "stim/draw/timeline_3d.h"

using namespace stim;
using namespace stim_internal;

constexpr size_t GL_FLOAT = 5126;
constexpr size_t GL_ARRAY_BUFFER = 34962;

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

struct VertexPrimitive {
    std::string name;
    std::vector<V3> vertices;
    size_t element_type;
    std::string material_name;

    std::pair<V3, V3> min_max() const {
        V3 v_min{INFINITY, INFINITY, INFINITY};
        V3 v_max{-INFINITY, -INFINITY, -INFINITY};
        for (const auto &v : vertices) {
            for (size_t k = 0; k < 3; k++) {
                v_min.xyz[k] = std::min(v_min.xyz[k], v.xyz[k]);
                v_max.xyz[k] = std::max(v_max.xyz[k], v.xyz[k]);
            }
        }
        return {v_min, v_max};
    }

    JsonObj primitive(size_t position_buffer_index, const std::map<std::string, size_t> material_index_map) const {
        return std::map<std::string, JsonObj>{
             {"attributes", {std::map<std::string, JsonObj>{{"POSITION", position_buffer_index}}}},
             {"material", material_index_map.at(material_name)},
             {"mode", element_type},
        };
    }

    JsonObj buffer() const {
        std::stringstream ss;
        ss << "data:application/octet-stream;base64,";
        size_t n = vertices.size() * sizeof(V3);
        write_base64((const char *)(const void *)vertices.data(), n, ss);
        return std::map<std::string, JsonObj>{
            {"name", name},
            {"uri", ss.str()},
            {"byteLength", n},
        };
    }

    JsonObj buffer_view(size_t index) const {
        return std::map<std::string, JsonObj>{
            {"name", name},
            {"buffer", index},
            {"byteOffset", 0},
            {"byteLength", vertices.size() * sizeof(V3)},
            {"target", GL_ARRAY_BUFFER},
        };
    }

    JsonObj buffer_view(const std::map<std::string, size_t> &index_map) const {
        return buffer_view(index_map.at(name));
    }

    JsonObj accessor(size_t index) const {
        auto mima = min_max();
        return std::map<std::string, JsonObj>{
            {"name", name},
            {"bufferView", index},
            {"byteOffset", 0},
            {"componentType", GL_FLOAT},
            {"count", vertices.size()},
            {"type", "VEC3"},
            {"min", std::vector<JsonObj>{mima.first.xyz[0], mima.first.xyz[1], mima.first.xyz[2]}},
            {"max", std::vector<JsonObj>{mima.second.xyz[0], mima.second.xyz[1], mima.second.xyz[2]}},
        };
    }

    JsonObj accessor(const std::map<std::string, size_t> &index_map) const {
        return accessor(index_map.at(name));
    }
};

std::string stim::circuit_diagram_timeline_3d(const Circuit &circuit) {
//    constexpr size_t GL_UNSIGNED_SHORT = 5123;
//    constexpr size_t GL_ELEMENT_ARRAY_BUFFER = 34963;
//    constexpr size_t GL_TRIANGLE_STRIP = 5;
    constexpr size_t GL_TRIANGLES = 4;
//    constexpr size_t GL_LINE_STRIP = 3;

    std::map<V3, size_t> color_to_material_index = {};
    std::vector<JsonObj> materials;

    materials.push_back(Material{
        .name="reddish",
        .base_color_factor_rgba={1, 0, 0.1, 1},
        .metallic_factor=0.1,
        .roughness_factor=0.5,
        .double_sided=true,
    }.to_json());
    materials.push_back(Material{
        .name="hyper_blue",
        .base_color_factor_rgba={0.1, 0, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=true,
    }.to_json());

    std::map<std::string, size_t> material_index_map;
    for (size_t k = 0; k < materials.size(); k++) {
        material_index_map.insert({materials[k].map.at("name").text, k});
    }

    VertexPrimitive first_triangle{"first_triangle", {
        {0, 0, 0},
        {0, 1, 0},
        {1, 0, 0},
    }, GL_TRIANGLES, "reddish"};
    VertexPrimitive second_triangle{"second_triangle", {
        {2, 0, 1},
        {0, 2, 1},
        {2, 2, 1},
    }, GL_TRIANGLES, "hyper_blue"};

    std::vector<VertexPrimitive> vertex_data_buffers{
        first_triangle,
        second_triangle,
    };
    std::vector<JsonObj> buffers;
    std::map<std::string, size_t> buffer_index_map;
    std::vector<JsonObj> buffer_views;
    std::vector<JsonObj> accessors;
    for (size_t k = 0; k < vertex_data_buffers.size(); k++) {
        const auto& v = vertex_data_buffers[k];
        buffers.push_back(v.buffer());
        buffer_index_map.insert({v.name, k});
        buffer_views.push_back(v.buffer_view(k));
        accessors.push_back(v.accessor(k));
    }

    std::vector<JsonObj> meshes;
    meshes.push_back(std::map<std::string, JsonObj>{
        {"primitives", std::vector<JsonObj>{
            first_triangle.primitive(buffer_index_map.at("first_triangle"), material_index_map),
        }}
    });
    meshes.push_back(std::map<std::string, JsonObj>{
        {"primitives", std::vector<JsonObj>{
            second_triangle.primitive(buffer_index_map.at("second_triangle"), material_index_map),
        }}
    });

    std::vector<JsonObj> scene_nodes;
    scene_nodes.push_back(0);
    scene_nodes.push_back(1);
    std::vector<JsonObj> nodes;
    nodes.push_back(std::map<std::string, JsonObj>{
        {"mesh", 0},
    });
    nodes.push_back(std::map<std::string, JsonObj>{
        {"mesh", 1},
    });

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
        {"buffers", buffers},
        {"bufferViews", std::move(buffer_views)},
        {"accessors", std::move(accessors)},
        {"materials", std::move(materials)},
    });
    return result.str();
}
