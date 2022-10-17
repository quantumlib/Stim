#include <iostream>

#include "stim/diagram/gltf.h"

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

void GltfSampler::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "samplers",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
}

JsonObj GltfSampler::to_json() const {
    return std::map<std::string, JsonObj>{
        {"magFilter", magFilter},
        {"minFilter", minFilter},
        {"wrapS", wrapS},
        {"wrapT", wrapT},
    };
}

void GltfImage::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "images",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
}

JsonObj GltfImage::to_json() const {
    return std::map<std::string, JsonObj>{
        {"name", id.name},
        {"uri", uri},
    };
}

void GltfTexture::visit(const gltf_visit_callback &callback) {
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

JsonObj GltfTexture::to_json() const {
    return std::map<std::string, JsonObj>{
        {"name", id.name},
        {"sampler", 0},
        {"source", 0},
    };
}

void GltfMaterial::visit(const gltf_visit_callback &callback) {
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

JsonObj GltfMaterial::to_json() const {
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

void GltfPrimitive::visit(const gltf_visit_callback &callback) {
    position_buffer->visit(callback);
    if (tex_coords_buffer) {
        tex_coords_buffer->visit(callback);
    }
    material->visit(callback);
}

JsonObj GltfPrimitive::to_json() const {
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

std::shared_ptr<GltfMesh> GltfMesh::from_singleton_primitive(std::shared_ptr<GltfPrimitive> primitive) {
    return std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_" + primitive->id.name},
        {primitive},
    });
}

void GltfMesh::visit(const gltf_visit_callback &callback) {
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

JsonObj GltfMesh::to_json() const {
    std::vector<JsonObj> json_primitives;
    for (const auto &p : primitives) {
        json_primitives.push_back(p->to_json());
    }
    return std::map<std::string, JsonObj>{
        {"primitives", std::move(json_primitives)},
    };
}

void GltfNode::visit(const gltf_visit_callback &callback) {
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

JsonObj GltfNode::to_json() const {
    return std::map<std::string, JsonObj>{
        {"mesh", mesh->id.index},
        {"translation", (std::vector<JsonObj>{translation.xyz[0], translation.xyz[1], translation.xyz[2]})},
    };
}
