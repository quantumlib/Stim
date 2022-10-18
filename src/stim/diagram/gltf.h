#ifndef _STIM_DRAW_3D_GLTF_H
#define _STIM_DRAW_3D_GLTF_H

#include <iostream>

#include "stim/diagram/coord.h"
#include "stim/diagram/json_obj.h"
#include "stim/mem/pointer_range.h"

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

    GltfId(std::string name) : name(name), index(SIZE_MAX) {
    }
    GltfId() = delete;
};

typedef std::function<void(GltfId &id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id)>
    gltf_visit_callback;

/// A named data buffer. Contains packed coordinate data.
template <size_t DIM>
struct GltfBuffer {
    GltfId id;
    std::vector<Coord<DIM>> vertices;

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
        auto mima = Coord<DIM>::min_max(vertices);
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

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfImage {
    GltfId id;
    std::string uri;

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfTexture {
    GltfId id;
    std::shared_ptr<GltfSampler> sampler;
    std::shared_ptr<GltfImage> source;

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfMaterial {
    GltfId id;
    std::array<float, 4> base_color_factor_rgba;
    float metallic_factor;
    float roughness_factor;
    bool double_sided;
    std::shared_ptr<GltfTexture> texture;

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfPrimitive {
    GltfId id;
    size_t element_type;
    std::shared_ptr<GltfBuffer<3>> position_buffer;
    std::shared_ptr<GltfBuffer<2>> tex_coords_buffer;
    std::shared_ptr<GltfMaterial> material;

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfMesh {
    GltfId id;
    std::vector<std::shared_ptr<GltfPrimitive>> primitives;

    static std::shared_ptr<GltfMesh> from_singleton_primitive(std::shared_ptr<GltfPrimitive> primitive);
    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfNode {
    GltfId id;
    std::shared_ptr<GltfMesh> mesh;
    Coord<3> translation;

    void visit(const gltf_visit_callback &callback);
    JsonObj to_json() const;
};

struct GltfScene {
    GltfId id;
    std::vector<std::shared_ptr<GltfNode>> nodes;

    void visit(const gltf_visit_callback &callback);
    JsonObj _to_json_local() const;
    JsonObj to_json();
};

}  // namespace stim_draw_internal

#endif
