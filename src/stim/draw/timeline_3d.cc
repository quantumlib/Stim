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

template <size_t N>
struct V {
    std::array<float, N> xyz;
    bool operator<(V<N> other) const {
        for (size_t k = 0; k < N; k++) {
            if (xyz[k] != other.xyz[k]) {
                return xyz[k] < other.xyz[k];
            }
        }
        return false;
    }
    bool operator==(V<N> other) const {
        return xyz == other.xyz;
    }
};

struct Triangle {
    V<3> c;
    std::array<V<3>, 3> v;
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
    size_t texture_index;

    std::pair<std::string, JsonObj> to_named_json() {
        return {name, to_json()};
    }

    JsonObj to_json() {
        JsonObj result = std::map<std::string, JsonObj>{
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
        if (texture_index != SIZE_MAX) {
            result.map.at("pbrMetallicRoughness").map.insert(
                {"baseColorTexture", std::map<std::string, JsonObj>{
                    {"index", texture_index},
                    {"texCoord", 0},
                }}
            );
        }
        return result;
    }
};

template <size_t N>
struct Buf {
    std::string name;
    std::vector<V<N>> vertices;

    std::pair<V<N>, V<N>> min_max() const {
        V<N> v_min;
        V<N> v_max;
        for (size_t k = 0; k < N; k++) {
            v_min.xyz[k] = INFINITY;
            v_max.xyz[k] = -INFINITY;
        }
        for (const auto &v : vertices) {
            for (size_t k = 0; k < N; k++) {
                v_min.xyz[k] = std::min(v_min.xyz[k], v.xyz[k]);
                v_max.xyz[k] = std::max(v_max.xyz[k], v.xyz[k]);
            }
        }
        return {v_min, v_max};
    }

    JsonObj buffer() const {
        std::stringstream ss;
        ss << "data:application/octet-stream;base64,";
        size_t n = vertices.size() * sizeof(V<N>);
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
            {"byteLength", vertices.size() * sizeof(V<N>)},
            {"target", GL_ARRAY_BUFFER},
        };
    }

    JsonObj buffer_view(const std::map<std::string, size_t> &index_map) const {
        return buffer_view(index_map.at(name));
    }

    JsonObj accessor(size_t index) const {
        auto mima = min_max();
        std::vector<JsonObj> min_v;
        std::vector<JsonObj> max_v;
        for (size_t k = 0; k < N; k++) {
            min_v.push_back(mima.first.xyz[k]);
            max_v.push_back(mima.second.xyz[k]);
        }
        return std::map<std::string, JsonObj>{
            {"name", name},
            {"bufferView", index},
            {"byteOffset", 0},
            {"componentType", GL_FLOAT},
            {"count", vertices.size()},
            {"type", "VEC" + std::to_string(N)},
            {"min", std::move(min_v)},
            {"max", std::move(max_v)},
        };
    }

    JsonObj accessor(const std::map<std::string, size_t> &index_map) const {
        return accessor(index_map.at(name));
    }
};

struct VertexPrimitive {
    std::string name;
    std::string positions_buf_name;
    std::string tex_coord_0_buf_name;
    size_t element_type;

    JsonObj primitive(
            const std::map<std::string, size_t> &buf_index_map,
            const std::map<std::string, size_t> &material_index_map,
            const std::string &material_name) const {
        std::map<std::string, JsonObj> attributes;
        attributes.insert({"POSITION", buf_index_map.at(positions_buf_name)});
        if (!tex_coord_0_buf_name.empty()) {
            attributes.insert({"TEXCOORD_0", buf_index_map.at(tex_coord_0_buf_name)});
        }
        return std::map<std::string, JsonObj>{
             {"attributes", std::move(attributes)},
             {"material", material_index_map.at(material_name)},
             {"mode", element_type},
        };
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
std::vector<V<3>> cube_triangle_strip() {
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

std::vector<V<3>> cube_triangle_list() {
    V<3> v000{-0.5f, +0.5f, +0.5f};
    V<3> v001{-0.5f, +0.5f, -0.5f};
    V<3> v010{-0.5f, -0.5f, +0.5f};
    V<3> v011{-0.5f, -0.5f, -0.5f};
    V<3> v100{+0.5f, +0.5f, +0.5f};
    V<3> v101{+0.5f, +0.5f, -0.5f};
    V<3> v110{+0.5f, -0.5f, +0.5f};
    V<3> v111{+0.5f, -0.5f, -0.5f};
    return {
        v000, v010, v100,
        v010, v110, v100,

        v000, v100, v001,
        v001, v100, v101,

        v000, v001, v010,
        v001, v011, v010,

        v111, v011, v101,
        v101, v011, v001,

        v111, v110, v011,
        v110, v010, v011,

        v111, v101, v110,
        v110, v101, v100,
    };
}

std::vector<V<2>> cube_triangle_list_st(size_t x, size_t y, size_t res, size_t diam) {
    float d = (float)1.0 / diam;
    float dx = d * x;
    float dy = d * y;
    V<2> v00{dx + 0, dy + 0};
    V<2> v01{dx + 0, dy + d};
    V<2> v10{dx + d, dy + 0};
    V<2> v11{dx + d, dy + d};

    return {
        v00, v01, v10,
        v01, v11, v10,

        v00, v00, v00,
        v00, v00, v00,

        v10, v00, v11,
        v00, v01, v11,

        v01, v11, v00,
        v00, v11, v10,

        v00, v00, v00,
        v00, v00, v00,

        v11, v10, v01,
        v01, v10, v00,
    };
}

std::vector<V<3>> circle_line_loop(size_t n) {
    std::vector<V<3>> result;
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
    std::vector<Material> materials;

    materials.push_back(Material{
        .name="reddish",
        .base_color_factor_rgba={1, 0, 0.1, 1},
        .metallic_factor=0.1,
        .roughness_factor=0.5,
        .double_sided=true,
        .texture_index=SIZE_MAX,
    });
    materials.push_back(Material{
        .name="hyper_blue",
        .base_color_factor_rgba={0.1, 0, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=true,
        .texture_index=SIZE_MAX,
    });
    materials.push_back(Material{
        .name="white",
        .base_color_factor_rgba={1, 1, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=true,
        .texture_index=SIZE_MAX,
    });
    materials.push_back(Material{
        .name="black",
        .base_color_factor_rgba={0, 0, 0, 1},
        .metallic_factor=1,
        .roughness_factor=1,
        .double_sided=true,
        .texture_index=SIZE_MAX,
    });
    materials.push_back(Material{
        .name="textured",
        .base_color_factor_rgba={1, 1, 1, 1},
        .metallic_factor=0.4,
        .roughness_factor=0.5,
        .double_sided=false,
        .texture_index=0,
    });

    std::map<std::string, size_t> material_index_map;
    for (size_t k = 0; k < materials.size(); k++) {
        material_index_map.insert({materials[k].name, k});
    }

    std::vector<Buf<3>> position_buffers{
        {"disk_triangle_fan", circle_line_loop(16)},
        {"circle_line_strip", circle_line_loop(16)},
        {"cube_triangle_strip", cube_triangle_strip()},
        {"cube_triangle_list", cube_triangle_list()},
        {"cross_lines", {{-0.5f, 0, 0}, {+0.5f, 0, 0}, {0, 0, -0.5f}, {0, 0, +0.5f}}},
        {"abs_lines", {}},
    };
    std::vector<Buf<2>> tex_coord_buffers{
        {"cube_tex_H", cube_triangle_list_st(0, 0, 32, 2)},
        {"cube_tex_SQRT_X", cube_triangle_list_st(1, 0, 32, 2)},
        {"cube_tex_S", cube_triangle_list_st(0, 1, 32, 2)},
        {"cube_tex_H_XY", cube_triangle_list_st(1, 1, 32, 2)},
    };
    std::map<std::string, size_t> buf_index_map;
    for (size_t k = 0; k < position_buffers.size(); k++) {
        const auto& v = position_buffers[k];
        buf_index_map.insert({v.name, k});
    }
    for (size_t k = 0; k < tex_coord_buffers.size(); k++) {
        const auto& v = tex_coord_buffers[k];
        buf_index_map.insert({v.name, k + position_buffers.size()});
    }

    std::vector<VertexPrimitive> primitives{
        VertexPrimitive{.name="disk", .positions_buf_name="disk_triangle_fan", .tex_coord_0_buf_name="", .element_type=GL_TRIANGLE_FAN},
        VertexPrimitive{.name="circle", .positions_buf_name="circle_line_strip", .tex_coord_0_buf_name="", .element_type=GL_LINE_STRIP},
        VertexPrimitive{.name="cube_H", .positions_buf_name="cube_triangle_list", .tex_coord_0_buf_name="cube_tex_H", .element_type=GL_TRIANGLES},
        VertexPrimitive{.name="cube_SQRT_X", .positions_buf_name="cube_triangle_list", .tex_coord_0_buf_name="cube_tex_SQRT_X", .element_type=GL_TRIANGLES},
        VertexPrimitive{.name="cube_S", .positions_buf_name="cube_triangle_list", .tex_coord_0_buf_name="cube_tex_S", .element_type=GL_TRIANGLES},
        VertexPrimitive{.name="cube_H_XY", .positions_buf_name="cube_triangle_list", .tex_coord_0_buf_name="cube_tex_H_XY", .element_type=GL_TRIANGLES},
        VertexPrimitive{.name="cross", .positions_buf_name="cross_lines", .tex_coord_0_buf_name="", .element_type=GL_LINES},
        VertexPrimitive{.name="abs_lines", .positions_buf_name="abs_lines", .tex_coord_0_buf_name="", .element_type=GL_LINES},
    };
    std::map<std::string, size_t> prim_index_map;
    for (size_t k = 0; k < primitives.size(); k++) {
        const auto& v = primitives[k];
        prim_index_map.insert({v.name, k});
    }

    std::vector<Mesh> meshes;
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
        "H",
        {"cube_H"},
        {"textured"}
    });
    meshes.push_back(Mesh{
        "S",
        {"cube_S"},
        {"textured"}
    });
    meshes.push_back(Mesh{
        "SQRT_X",
        {"cube_SQRT_X"},
        {"textured"}
    });
    meshes.push_back(Mesh{
        "H_XY",
        {"cube_H_XY"},
        {"textured"}
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
        {"mesh", mesh_index_map.at("H")},
        {"translation", std::vector<JsonObj>{
            0,
            2,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("H_XY")},
        {"translation", std::vector<JsonObj>{
            3,
            2,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("S")},
        {"translation", std::vector<JsonObj>{
            5,
            2,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("SQRT_X")},
        {"translation", std::vector<JsonObj>{
            7,
            2,
            0,
        }},
    });
    nodes_json.push_back(std::map<std::string, JsonObj>{
        {"mesh", mesh_index_map.at("abs_lines")},
    });

    {
        auto &v = position_buffers[buf_index_map.at("abs_lines")].vertices;
        v.push_back({0, 0, 0});
        v.push_back({2, 0, 0});
    }

    std::vector<JsonObj> json_meshes;
    for (auto &m : meshes) {
        json_meshes.push_back(m.to_json(
            primitives,
            prim_index_map,
            buf_index_map,
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
    for (size_t k = 0; k < position_buffers.size(); k++) {
        const auto& v = position_buffers[k];
        buffers_json.push_back(v.buffer());
        buffer_views_json.push_back(v.buffer_view(k));
        accessors_json.push_back(v.accessor(k));
    }
    for (size_t k = 0; k < tex_coord_buffers.size(); k++) {
        size_t k2 = k + position_buffers.size();
        const auto& v = tex_coord_buffers[k];
        buffers_json.push_back(v.buffer());
        buffer_views_json.push_back(v.buffer_view(k2));
        accessors_json.push_back(v.accessor(k2));
    }

    std::vector<JsonObj> materials_json;
    for (size_t k = 0; k < materials.size(); k++) {
        materials_json.push_back(materials[k].to_json());
    }

    std::vector<JsonObj> textures_json;
    textures_json.push_back(std::map<std::string, JsonObj>{
        {"sampler", 0},
        {"source", 0},
    });

    std::vector<JsonObj> images_json;
    images_json.push_back(std::map<std::string, JsonObj>{
        {"uri", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAIAAAAlC+aJAAAFG0lEQVRo3u2ZfUxVdRjHPxcucnm9QCKIFxBock0mdg2bM4oy8HXKKMK2VLC2ItM/etlYMUSHm7oSYroxNdebrUABhQkZunTQWjqFmib3DhWFAIUguLzI2+2Pjl1goOi553juxrP7x3N/5zk753t+z+95+6osFiQV1XZpH+CAnYvdA1DL/cDBuzTX0HiRxgu0GjG34B+JVoe7H756nghDG4hGq1QAf12kPI2bP41a7DSONUvrmDyG8V0oNRWVCpUKk+kB9zc0CJZr1jzoUdfPcmDh2Le3Gxcy3+aHNwQ9eBnPvY9fBK4+OE5jqJ+BXvq76WzkTi31VahUygNwo5K+BgD9OuLzRnmIWoNag4s3Wh2Bz2LYoMgo1HRJUBZveagzqhgA5hZBcfWxzzzg5isoHTftE4AuSlDKPqK11g4BhMYw3QDw9+/s01OcyuVCmmrobgVRxZJcUUjjRdJ3HE2m5VeA6jyq84RL/kuYF8+shQQ8jcbLxgDmzLEdBt9wkk/y5wnO7aHjinW9uYrmKgBHD1bmMv81nFyVWkq4eGPYyPx1tJm4Y6S5htoy7pwXrg51UZLCtTOs2Yezp1KLOUDtjF8EfhFEJLA0k55WWo38kc+FXIDL3+CrJ+Zj2xxioxGL5X6/W7dE9jsq3HwJXsKqbF49Kiz+/AldzfbWD6gcmBdPyCrhb/t1QbnwBYfj6DdbLS0WKrZxYgvDgwpraFSOBBgE/e69N458HcdpVB8ZUVad47c8YtJwUCsMgGWYpmpBd3YXFCdX4nZy8h1uXwHoaaNkKwmH8ZylvJaytoxrJYKuDbSuz4wkNoeKTAZ6+CWXkBeYs0z2KFS5F8swM+biNRtXH6GEVmuwDHG3i46bXC3lbLpgvOgDtLpRtz+TwuVjFL+LqYz3Lv3nPPIC6G2nKmtSlkGxRH84dtHZk5e38/VLvJKPZ4CCpxLRmSQdwcN/nONhOgVg+pGhgceRyF5MJ+ot/mmgrY72G3S30F5PUw1aHd6zmRnJjHkELBjl+iOl7gyXvuXtago2UHuSp9bKDkDtjFcwXsEEL3mUZujEZhIOMDOS1Z/zfRK6qP8dSfGDLcswlXsJX8WTcQAhz2NIofIzLENC8lD6bNR0imNvsvm89WB0NbM/isQvCVuq+B0wt1CcSsKhUcfaw5/4PEq20t1qDztgN8XcI38gMqX9QpZtqqkdmAIwBUDKFP9gE9GcSmUl0dEAnZ14eIxvc/w48fFCw2hTABJwKjLuwPWzfBVjty4kGaciFwDJOBW5opBknIpcACTjVOQCIBmnItcZGMmpeAczPVz8wzw95dyBUKk4Fbl2QAJOZTKZ2KaJTBpORd5aSAJORfZiDhtzKo+7nBbNqSimH5iIU7GnhmZcTkWcDAyQksKVeyGjv5/kZK5eZf9+ioqsZvX1rF5Nd7dIAONyKuLEyYn0dDIy6O0FKCwkJga9no0bOXiQhgaAwUF27SIrCzc3kQAm4lTESVgYa9dSUEBdHYWFJCUBuLuzcye7dzM4SHk5c+eyYMH950IPy6ms/NSGc6HeXtavx2wmJwe93rqek4NGQ3Ex+flCYTJxGBXJqYgTFxdiYykqIjR01PqmTSxezKFD1rJK9FRiIk5FnJhMnD7NihWUlo4tB8PC8PefTCITyamIkL4+MjLYsYPAQBITWbQIne4RMrEYTkWcFBSwfLng+llZ7NlDdjaOjnYy2DIaKS0VIg9gMBAUREXFxKloajr9mOVfOIsiZzSiOxcAAAAASUVORK5CYII="}
    });

//    constexpr size_t GL_REPEAT = 10497;
//    constexpr size_t GL_CLAMP = 10496;
    constexpr size_t GL_CLAMP_TO_EDGE = 33071;
//    constexpr size_t GL_LINEAR = 9729;
//    constexpr size_t GL_LINEAR_MIPMAP_NEAREST = 9987;
    constexpr size_t GL_NEAREST = 9728;
    std::vector<JsonObj> samplers_json;
    samplers_json.push_back(std::map<std::string, JsonObj>{
        {"magFilter", GL_NEAREST},
        {"minFilter", GL_NEAREST},
        {"wrapS", GL_CLAMP_TO_EDGE},
        {"wrapT", GL_CLAMP_TO_EDGE},
    });

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
        {"textures", std::move(textures_json)},
        {"samplers", std::move(samplers_json)},
        {"images", std::move(images_json)},
        {"nodes", std::move(nodes_json)},
        {"meshes", std::move(json_meshes)},
        {"buffers", std::move(buffers_json)},
        {"bufferViews", std::move(buffer_views_json)},
        {"accessors", std::move(accessors_json)},
        {"materials", std::move(materials_json)},
    });
    return result.str();
}
