#include "stim/draw/diagram.h"
#include "stim/draw/json_obj.h"
#include "stim/draw/timeline_3d.h"

using namespace stim;
using namespace stim_internal;

constexpr size_t GL_FLOAT = 5126;
constexpr size_t GL_ARRAY_BUFFER = 34962;
//    constexpr size_t GL_UNSIGNED_SHORT = 5123;
//    constexpr size_t GL_ELEMENT_ARRAY_BUFFER = 34963;
    constexpr size_t GL_TRIANGLE_STRIP = 5;
    constexpr size_t GL_TRIANGLES = 4;
    constexpr size_t GL_TRIANGLE_FAN = 6;

    constexpr size_t GL_LINES = 1;
    constexpr size_t GL_LINE_STRIP = 3;
//    constexpr size_t GL_LINE_LOOP = 2;

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

    JsonObj primitive(size_t position_buffer_index, const std::map<std::string, size_t> &material_index_map, const std::string &material_name) const {
        return std::map<std::string, JsonObj>{
             {"attributes", {std::map<std::string, JsonObj>{{"POSITION", position_buffer_index}}}},
            {"material", material_index_map.at(material_name)},
             {"mode", element_type},
        };
    }

    JsonObj primitive(const std::map<std::string, size_t> &buffer_index_map, const std::map<std::string, size_t> &material_index_map, const std::string &material_name) const {
        return primitive(buffer_index_map.at(name), material_index_map, material_name);
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

/// @inproceedings{evans1996optimizing,
///     title={Optimizing triangle strips for fast rendering},
///     author={Evans, Francine and Skiena, Steven and Varshney, Amitabh},
///     booktitle={Proceedings of Seventh Annual IEEE Visualization'96},
///     pages={319--326},
///     year={1996},
///     organization={IEEE}
/// }
std::vector<V3> cube_triangle_strip() {
    return {
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, +0.5f, -0.5f},
        {+0.5f, -0.5f, -0.5f},
        {+0.5f, +0.5f, -0.5f},
        {+0.5f, +0.5f, +0.5f},
        {-0.5f, +0.5f, -0.5f},
        {-0.5f, +0.5f, +0.5f},
        {-0.5f, -0.5f, +0.5f},
        {+0.5f, +0.5f, +0.5f},
        {+0.5f, -0.5f, +0.5f},
        {+0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, +0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, +0.5f, -0.5f},
    };
}

std::vector<V3> circle_line_loop(size_t n) {
    std::vector<V3> result;
    result.push_back({0.5f, 0, 0});
    for (size_t k = 1; k < n; k++) {
        float t = k*M_PI*2/n;
        result.push_back({cosf(t) * 0.5f, 0, sinf(t) * 0.5f});
    }
    result.push_back({0.5f, 0, 0});
    return result;
}

struct Mesh {
    std::string name;
    std::vector<std::string> primitive_names;
    std::vector<std::string> material_names;

    JsonObj to_json(const std::vector<VertexPrimitive> &primitives,
              const std::map<std::string, size_t> primitive_index_map,
              const std::map<std::string, size_t> buffer_index_map,
              const std::map<std::string, size_t> material_index_map) {
        std::vector<JsonObj> json_primitives;
        for (size_t k = 0; k < material_names.size(); k++) {
            auto &p = primitives[primitive_index_map.at(primitive_names[k])];
            json_primitives.push_back(
                p.primitive(buffer_index_map, material_index_map, material_names[k])
            );
        }
        return std::map<std::string, JsonObj>{
            {"primitives", std::move(json_primitives)},
        };
    }
};

std::string stim::circuit_diagram_timeline_3d(const Circuit &circuit) {
    std::map<V3, size_t> color_to_material_index = {};
    std::vector<Material> materials;

    materials.push_back(Material{
        .name="reddish",
        .base_color_factor_rgba={1, 0, 0.1, 1},
        .metallic_factor=0.1,
        .roughness_factor=0.5,
        .double_sided=true,
    });
    materials.push_back(Material{
        .name="hyper_blue",
        .base_color_factor_rgba={0.1, 0, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=true,
    });
    materials.push_back(Material{
        .name="white",
        .base_color_factor_rgba={1, 1, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=true,
    });
    materials.push_back(Material{
        .name="black",
        .base_color_factor_rgba={0, 0, 0, 1},
        .metallic_factor=0,
        .roughness_factor=0.95,
        .double_sided=true,
    });
    materials.push_back(Material{
        .name="yellow",
        .base_color_factor_rgba={1, 1, 0, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=false,
    });


    std::map<std::string, size_t> material_index_map;
    for (size_t k = 0; k < materials.size(); k++) {
        material_index_map.insert({materials[k].name, k});
    }

    std::vector<VertexPrimitive> primitives{
        {"disk", circle_line_loop(16), GL_TRIANGLE_FAN},
        {"circle", circle_line_loop(16), GL_LINE_STRIP},
        {"cube", cube_triangle_strip(), GL_TRIANGLE_STRIP},
        {"cross", {{-0.5f, 0, 0}, {+0.5f, 0, 0}, {0, 0, -0.5f}, {0, 0, +0.5f}}, GL_LINES},
        {"abs_lines", {}, GL_LINES},
    };
    std::map<std::string, size_t> buffer_index_map;
    for (size_t k = 0; k < primitives.size(); k++) {
        const auto& v = primitives[k];
        buffer_index_map.insert({v.name, k});
    }

    std::vector<Mesh> meshes;
    meshes.push_back(Mesh{
        "insta_disk",
        {"disk"},
        {"reddish"}
    });
    meshes.push_back(Mesh{
        "insta_cube",
        {"cube"},
        {"yellow"}
    });
    meshes.push_back(Mesh{
        "X",
        {"disk", "circle", "cross"},
        {"white", "black", "black"}
    });
    meshes.push_back(Mesh{
        "@",
        {"disk"},
        {"black"}
    });
    meshes.push_back(Mesh{
        "abs_lines",
        {"abs_lines"},
        {"black"}
    });

    std::map<std::string, size_t> mesh_index_map;
    for (size_t k = 0; k < meshes.size(); k++) {
        mesh_index_map.insert({meshes[k].name, k});
    }

    std::vector<JsonObj> nodes_json;
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("X")},
        {"translation", std::vector<JsonObj>{
            0,
            0,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("@")},
        {"translation", std::vector<JsonObj>{
            2,
            0,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("abs_lines")},
    });
    {
        auto &v = primitives[buffer_index_map.at("abs_lines")].vertices;
        v.push_back({0, 0, 0});
        v.push_back({2, 0, 0});
    }

    std::vector<JsonObj> json_meshes;
    for (auto &m : meshes) {
        json_meshes.push_back(m.to_json(
            primitives,
            buffer_index_map,
            buffer_index_map,
            material_index_map
        ));
    }

    std::vector<JsonObj> scene_nodes_json;
    for (size_t k = 0; k < nodes_json.size(); k++) {
        scene_nodes_json.push_back(k);
    }

    std::vector<JsonObj> buffers_json;
    std::vector<JsonObj> buffer_views_json;
    std::vector<JsonObj> accessors_json;
    for (size_t k = 0; k < primitives.size(); k++) {
        const auto& v = primitives[k];
        buffers_json.push_back(v.buffer());
        buffer_views_json.push_back(v.buffer_view(k));
        accessors_json.push_back(v.accessor(k));
    }

    std::vector<JsonObj> materials_json;
    for (size_t k = 0; k < materials.size(); k++) {
        materials_json.push_back(materials[k].to_json());
    }

    JsonObj result(std::map<std::string, JsonObj>{
        {"scene", 0},
        {"scenes", std::vector<JsonObj>{
            std::map<std::string, JsonObj>{
                {"nodes", std::move(scene_nodes_json)},
            },
        }},
        {"asset", std::map<std::string, JsonObj>{
            {"version", "2.0"},
        }},
        {"nodes", std::move(nodes_json)},
        {"meshes", std::move(json_meshes)},
        {"buffers", std::move(buffers_json)},
        {"bufferViews", std::move(buffer_views_json)},
        {"accessors", std::move(accessors_json)},
        {"materials", std::move(materials_json)},
    });
    return result.str();
}
