#include "stim/draw/diagram.h"
#include "stim/draw/timeline_3d.h"

using namespace stim;

JsonObj::JsonObj(int num) : num(num), type(0) {
}
JsonObj::JsonObj(size_t num) : num(num), type(0) {
}
JsonObj::JsonObj(double num) : num(num), type(0) {
}

JsonObj::JsonObj(std::string text) : text(text), type(1) {
}

JsonObj::JsonObj(const char *text) : text(text), type(1) {
}

JsonObj::JsonObj(std::map<std::string, JsonObj> map) : map(map), type(2) {
}

JsonObj::JsonObj(std::vector<JsonObj> arr) : arr(arr), type(3) {
}

JsonObj::~JsonObj() {
    if (type == 1) {
        text.~basic_string();
    } else if (type == 2) {
        map.~map<std::string, JsonObj>();
    } else if (type == 3) {
        arr.~vector<JsonObj>();
    }
    type = 0;
}

JsonObj::JsonObj(JsonObj &&other) noexcept {
    if (this == &other) {
        return;
    }

    (*this).~JsonObj();
    if (other.type == 0) {
        new (this) JsonObj(std::move(other.num));
    } else if (other.type == 1) {
        new (this) JsonObj(std::move(other.text));
    } else if (other.type == 2) {
        new (this) JsonObj(std::move(other.map));
    } else if (other.type == 3) {
        new (this) JsonObj(std::move(other.arr));
    }
}

JsonObj::JsonObj(const JsonObj &other) {
    if (this == &other) {
        return;
    }

    (*this).~JsonObj();
    if (other.type == 0) {
        new (this) JsonObj(other.num);
    } else if (other.type == 1) {
        new (this) JsonObj(other.text);
    } else if (other.type == 2) {
        new (this) JsonObj(other.map);
    } else if (other.type == 3) {
        new (this) JsonObj(other.arr);
    }
}

void JsonObj::write_str(const std::string &s, std::ostream &out) {
    out << '"';
    for (char c : s) {
        if (c == '\0') {
            out << "\\0";
        } else if (c == '\n') {
            out << "\\n";
        } else if (c == '"') {
            out << "\\\"";
        } else if (c == '\\') {
            out << "\\\\";
        } else {
            out << c;
        }
    }
    out << '"';
}

void JsonObj::write(std::ostream &out) const {
    if (type == 0) {
        out << num;
    } else if (type == 1) {
        write_str(text, out);
    } else if (type == 2) {
        out << "{";
        bool first = true;
        for (const auto &e : map) {
            if (first) {
                first = false;
            } else {
                out << ',';
            }
            write_str(e.first, out);
            out << ':';
            e.second.write(out);
        }
        out << "}";
    } else if (type == 3) {
        out << "[";
        bool first = true;
        for (const auto &e : arr) {
            if (first) {
                first = false;
            } else {
                out << ',';
            }
            e.write(out);
        }
        out << "]";
    } else {
        throw std::invalid_argument("unknown type");
    }
}

std::string JsonObj::str() const {
    std::stringstream ss;
    write(ss);
    return ss.str();
}

struct V3 {
    std::array<double, 3> xyz;
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

char v2b64(uint8_t v) {
    if (v < 26) {
        return 'A' + v;
    } else if (v < 52) {
        return 'a' + v - 26;
    } else if (v < 62) {
        return '0' + v - 52;
    } else if (v == 62) {
        return '+';
    } else {
        return '/';
    }
}

void stim::write_base64(const char *data, size_t n, std::ostream &out) {
    uint32_t buf = 0;
    size_t bits_in_buf = 0;
    for (size_t k = 0; k < n; k++) {
        buf <<= 8;
        buf |= (uint8_t)data[k];
        bits_in_buf += 8;
        if (bits_in_buf == 24) {
            out << v2b64((buf >> 18) & 0x3F);
            out << v2b64((buf >> 12) & 0x3F);
            out << v2b64((buf >> 6) & 0x3F);
            out << v2b64((buf >> 0) & 0x3F);
            bits_in_buf = 0;
            buf = 0;
        }
    }

    if (bits_in_buf) {
        buf <<= (24 - bits_in_buf);
        out << v2b64((buf >> 18) & 0x3F);
        out << v2b64((buf >> 12) & 0x3F);
        out << (bits_in_buf == 8 ? '=' : v2b64((buf >> 6) & 0x3F));
        out << '=';
    }
}


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
    std::vector<JsonObj> meshes;
    std::vector<std::vector<std::array<V3, 3>>> triangle_lists;
    std::vector<float> vert_data;
    for (const auto &t : triangles) {
        auto color_key = t.c;
        if (color_to_material_index.find(color_key) == color_to_material_index.end()) {
            triangle_lists.push_back({});
            color_to_material_index[color_key] = materials.size();
            materials.push_back(std::map<std::string, JsonObj>{
                {"pbrMetallicRoughness", std::map<std::string, JsonObj>{
                    {"baseColorFactor", std::vector<JsonObj>{t.c.xyz[0], t.c.xyz[1], t.c.xyz[2], 1.0}},
                    {"metallicFactor", 0.5},
                    {"roughnessFactor", 0.1},
                }},
            });
        }
        auto color_id = color_to_material_index.at(color_key);
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
        triangle_lists[color_id].push_back(t.v);
    }

    std::vector<JsonObj> buffer_views{
        std::map<std::string, JsonObj>{
            {"buffer", 0},
            {"byteOffset", 0},
            {"byteLength", vert_data.size() * sizeof(float)},
            {"target", GL_ARRAY_BUFFER},
        },
    };

    std::vector<JsonObj> accessors{
        std::map<std::string, JsonObj>{
            {"bufferView", 0},
            {"byteOffset", 0},
            {"componentType", GL_FLOAT},
            {"count", vert_data.size() / 3},
            {"type", "VEC3"},
            {"min", std::vector<JsonObj>{-100, -100, -100}},
            {"max", std::vector<JsonObj>{100, 100, 100}},
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
            index_data.push_back(c);
            index_data.push_back(b);
            index_data.push_back(a);
        }

        meshes.push_back(std::map<std::string, JsonObj>{
            {"primitives",
             std::vector<JsonObj>{{std::map<std::string, JsonObj>{
                 {"attributes", {std::map<std::string, JsonObj>{{"POSITION", 0}}}},
                 {"indices", mesh_index + 1},
                 {"material", mesh_index},
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
            {"count", tlist.size() * 6},
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
