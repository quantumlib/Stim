#ifndef _STIM_DRAW_3D_GLTF_H
#define _STIM_DRAW_3D_GLTF_H

#include <iostream>

#include "stim/draw/3d/json_obj.h"

namespace stim_draw_internal {

constexpr size_t GL_FLOAT = 5126;
constexpr size_t GL_ARRAY_BUFFER = 34962;
constexpr size_t GL_UNSIGNED_SHORT = 5123;
constexpr size_t GL_ELEMENT_ARRAY_BUFFER = 34963;
constexpr size_t GL_TRIANGLE_STRIP = 5;
constexpr size_t GL_TRIANGLES = 4;
constexpr size_t GL_TRIANGLE_FAN = 6;

constexpr size_t GL_LINES = 1;
constexpr size_t GL_LINE_STRIP = 3;
constexpr size_t GL_LINE_LOOP = 2;

constexpr size_t GL_REPEAT = 10497;
constexpr size_t GL_CLAMP = 10496;
constexpr size_t GL_CLAMP_TO_EDGE = 33071;
constexpr size_t GL_LINEAR = 9729;
constexpr size_t GL_LINEAR_MIPMAP_NEAREST = 9987;
constexpr size_t GL_NEAREST = 9728;

struct GltfId {
    std::string name;
    size_t index;

    GltfId(std::string name) : name(name), index(SIZE_MAX) {}
    GltfId() = delete;
};

typedef std::function<void(GltfId &id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id)>
    gltf_visit_callback;

/// Coordinate data. Used for individual vertex positions and also UV texture coordinates.
template <size_t DIM>
struct Coord {
    std::array<float, DIM> xyz;

    bool operator<(Coord<DIM> other) const {
        for (size_t k = 0; k < DIM; k++) {
            if (xyz[k] != other.xyz[k]) {
                return xyz[k] < other.xyz[k];
            }
        }
        return false;
    }

    bool operator==(Coord<DIM> other) const {
        return xyz == other.xyz;
    }
};

/// A named data buffer. Contains packed coordinate data.
template <size_t DIM>
struct GltfBuffer {
    GltfId id;
    std::vector<Coord<DIM>> vertices;

    std::pair<Coord<DIM>, Coord<DIM>> min_max() const {
        Coord<DIM> v_min;
        Coord<DIM> v_max;
        for (size_t k = 0; k < DIM; k++) {
            v_min.xyz[k] = INFINITY;
            v_max.xyz[k] = -INFINITY;
        }
        for (const auto &v : vertices) {
            for (size_t k = 0; k < DIM; k++) {
                v_min.xyz[k] = std::min(v_min.xyz[k], v.xyz[k]);
                v_max.xyz[k] = std::max(v_max.xyz[k], v.xyz[k]);
            }
        }
        return {v_min, v_max};
    }

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "buffers",
            [&]() {
                return to_json_buffer();
            },
            (uintptr_t)this);
        callback(
            id,
            "bufferViews",
            [&]() {
                return to_json_buffer_view();
            },
            (uintptr_t)this);
        callback(
            id,
            "accessors",
            [&]() {
                return to_json_accessor();
            },
            (uintptr_t)this);
    }

    JsonObj to_json_buffer() const {
        std::stringstream ss;
        ss << "data:application/octet-stream;base64,";
        size_t n = vertices.size() * sizeof(Coord<DIM>);
        write_base64((const char *)(const void *)vertices.data(), n, ss);
        return std::map<std::string, JsonObj>{
            {"name", id.name},
            {"uri", ss.str()},
            {"byteLength", n},
        };
    }

    JsonObj to_json_buffer_view() const {
        return std::map<std::string, JsonObj>{
            {"name", id.name},
            {"buffer", id.index},
            {"byteOffset", 0},
            {"byteLength", vertices.size() * sizeof(Coord<DIM>)},
            {"target", GL_ARRAY_BUFFER},
        };
    }

    JsonObj to_json_accessor() const {
        auto mima = min_max();
        std::vector<JsonObj> min_v;
        std::vector<JsonObj> max_v;
        for (size_t k = 0; k < DIM; k++) {
            min_v.push_back(mima.first.xyz[k]);
            max_v.push_back(mima.second.xyz[k]);
        }
        return std::map<std::string, JsonObj>{
            {"name", id.name},
            {"bufferView", id.index},
            {"byteOffset", 0},
            {"componentType", GL_FLOAT},
            {"count", vertices.size()},
            {"type", "VEC" + std::to_string(DIM)},
            {"min", std::move(min_v)},
            {"max", std::move(max_v)},
        };
    }
};

struct GltfSampler {
    GltfId id;
    size_t magFilter;
    size_t minFilter;
    size_t wrapS;
    size_t wrapT;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "samplers",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
    }

    JsonObj to_json() const {
        return std::map<std::string, JsonObj>{
            {"magFilter", magFilter},
            {"minFilter", minFilter},
            {"wrapS", wrapS},
            {"wrapT", wrapT},
        };
    }
};

struct GltfImage {
    GltfId id;
    std::string uri;
//    std::vector<uint8_t> data;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "images",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
    }

    JsonObj to_json() const {
//        std::stringstream ss;
//        ss << "data:application/octet-stream;base64,";
//        write_base64((const char *)(const void *)data.data(), data.size(), ss);
//        auto uri = ss.str();
        return std::map<std::string, JsonObj>{
            {"name", id.name},
            {"uri", uri},
        };
    }
};

struct GltfTexture {
    GltfId id;
    std::shared_ptr<GltfSampler> sampler;
    std::shared_ptr<GltfImage> source;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "textures",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
        sampler->visit(callback);
        source->visit(callback);
    }

    JsonObj to_json() const {
        return std::map<std::string, JsonObj>{
            {"name", id.name},
            {"sampler", 0},
            {"source", 0},
        };
    }
};

struct GltfMaterial {
    GltfId id;
    std::array<float, 4> base_color_factor_rgba;
    float metallic_factor;
    float roughness_factor;
    bool double_sided;
    std::shared_ptr<GltfTexture> texture;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "materials",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
        if (texture) {
            texture->visit(callback);
        }
    }

    JsonObj to_json() const {
        JsonObj result = std::map<std::string, JsonObj>{
            {"name", id.name},
            {"pbrMetallicRoughness",
             std::map<std::string, JsonObj>{
                 {"baseColorFactor",
                  std::vector<JsonObj>{
                      base_color_factor_rgba[0],
                      base_color_factor_rgba[1],
                      base_color_factor_rgba[2],
                      base_color_factor_rgba[3],
                  }},
                 {"metallicFactor", metallic_factor},
                 {"roughnessFactor", roughness_factor}}},
            {"doubleSided", double_sided},
        };
        if (texture) {
            result.map.at("pbrMetallicRoughness")
                .map.insert(
                    {"baseColorTexture",
                     std::map<std::string, JsonObj>{
                         {"index", texture->id.index},
                         {"texCoord", 0},
                     }});
        }
        return result;
    }
};

struct GltfPrimitive {
    GltfId id;
    size_t element_type;
    std::shared_ptr<GltfBuffer<3>> position_buffer;
    std::shared_ptr<GltfBuffer<2>> tex_coords_buffer;
    std::shared_ptr<GltfMaterial> material;

    void visit(const gltf_visit_callback &callback) {
        position_buffer->visit(callback);
        if (tex_coords_buffer) {
            tex_coords_buffer->visit(callback);
        }
        material->visit(callback);
    }

    JsonObj to_json() const {
        std::map<std::string, JsonObj> attributes;
        attributes.insert({"POSITION", position_buffer->id.index});
        if (tex_coords_buffer) {
            attributes.insert({"TEXCOORD_0", tex_coords_buffer->id.index});
        }
        return std::map<std::string, JsonObj>{
            // Note: validator says "name" not expected for primitives.
            // {"name", id.name},
            {"attributes", std::move(attributes)},
            {"material", material->id.index},
            {"mode", element_type},
        };
    }
};

struct GltfMesh {
    GltfId id;
    std::vector<std::shared_ptr<GltfPrimitive>> primitives;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "meshes",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
        for (auto &p : primitives) {
            p->visit(callback);
        }
    }

    JsonObj to_json() const {
        std::vector<JsonObj> json_primitives;
        for (const auto &p : primitives) {
            json_primitives.push_back(p->to_json());
        }
        return std::map<std::string, JsonObj>{
            {"primitives", std::move(json_primitives)},
        };
    }
};

struct GltfNode {
    GltfId id;
    std::shared_ptr<GltfMesh> mesh;
    Coord<3> translation;

    void visit(const gltf_visit_callback &callback) {
        callback(
            id,
            "nodes",
            [&]() {
                return to_json();
            },
            (uintptr_t)this);
        if (mesh) {
            mesh->visit(callback);
        }
    }

    JsonObj to_json() const {
        return std::map<std::string, JsonObj>{
            {"mesh", mesh->id.index},
            {"translation", (std::vector<JsonObj>{translation.xyz[0], translation.xyz[1], translation.xyz[2]})},
        };
    }
};

struct GltfScene {
    GltfId id;
    std::vector<std::shared_ptr<GltfNode>> nodes;

    void visit(const gltf_visit_callback &callback) {
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

    JsonObj _to_json_local() const {
        std::vector<JsonObj> scene_nodes_json;
        for (const auto &n : nodes) {
            scene_nodes_json.push_back(n->id.index);
        }
        return std::map<std::string, JsonObj>{
            {"nodes", std::move(scene_nodes_json)},
        };
    }

    JsonObj to_json() {
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
};

}

#endif
