#include "stim/diagram/json_obj.h"

#include <limits>

using namespace stim;
using namespace stim_draw_internal;

enum JsonType : uint8_t {
    JsonTypeEmpty = 0,
    JsonTypeMap = 1,
    JsonTypeArray = 2,
    JsonTypeBool = 3,
    JsonTypeSingle = 4,
    JsonTypeDouble = 5,
    JsonTypeInt64 = 6,
    JsonTypeUInt64 = 7,
    JsonTypeString = 8,
};

JsonObj::JsonObj(float num) : val_float(num), type(JsonTypeSingle) {
}
JsonObj::JsonObj(double num) : val_double(num), type(JsonTypeDouble) {
}
JsonObj::JsonObj(uint64_t num) : val_uint64_t(num), type(JsonTypeUInt64) {
}
JsonObj::JsonObj(uint32_t num) : val_uint64_t(num), type(JsonTypeUInt64) {
}
JsonObj::JsonObj(uint16_t num) : val_uint64_t(num), type(JsonTypeUInt64) {
}
JsonObj::JsonObj(uint8_t num) : val_uint64_t(num), type(JsonTypeUInt64) {
}
JsonObj::JsonObj(int8_t num) : val_int64_t(num), type(JsonTypeInt64) {
}
JsonObj::JsonObj(int16_t num) : val_int64_t(num), type(JsonTypeInt64) {
}
JsonObj::JsonObj(int32_t num) : val_int64_t(num), type(JsonTypeInt64) {
}
JsonObj::JsonObj(int64_t num) : val_int64_t(num), type(JsonTypeInt64) {
}

JsonObj::JsonObj(std::string text) : text(text), type(JsonTypeString) {
}

JsonObj::JsonObj(const char *text) : text(text), type(JsonTypeString) {
}

JsonObj::JsonObj(std::map<std::string, JsonObj> map) : map(map), type(JsonTypeMap) {
}

JsonObj::JsonObj(std::vector<JsonObj> arr) : arr(arr), type(JsonTypeArray) {
}

JsonObj::JsonObj(bool boolean) : val_boolean(boolean), type(JsonTypeBool) {
}

void JsonObj::clear() {
    text.clear();
    map.clear();
    arr.clear();
    type = JsonTypeEmpty;
    val_uint64_t = 0;
}

void JsonObj::write_str(std::string_view s, std::ostream &out) {
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

void indented_new_line(std::ostream &out, int64_t indent) {
    if (indent >= 0) {
        out << '\n';
        for (int64_t k = 0; k < indent; k++) {
            out << ' ';
        }
    }
}

void JsonObj::write(std::ostream &out, int64_t indent) const {
    switch (type) {
        case JsonTypeMap: {
            out << "{";
            indented_new_line(out, indent + 2);
            bool first = true;
            for (const auto &e : map) {
                if (first) {
                    first = false;
                } else {
                    out << ',';
                    indented_new_line(out, indent + 2);
                }
                write_str(e.first, out);
                out << ':';
                e.second.write(out, indent + 2);
            }
            if (!first) {
                indented_new_line(out, indent);
            }
            out << "}";
            break;
        }
        case JsonTypeArray: {
            out << "[";
            indented_new_line(out, indent + 2);
            bool first = true;
            for (const auto &e : arr) {
                if (first) {
                    first = false;
                } else {
                    out << ',';
                    indented_new_line(out, indent + 2);
                }
                e.write(out, indent + 2);
            }
            if (!first) {
                indented_new_line(out, indent);
            }
            out << "]";
            break;
        }
        case JsonTypeString: {
            write_str(text, out);
            break;
        }
        case JsonTypeBool: {
            out << (val_boolean ? "true" : "false");
            break;
        }
        case JsonTypeSingle: {
            out << val_float;
            break;
        }
        case JsonTypeDouble: {
            auto p = out.precision();
            out.precision(std::numeric_limits<double>::digits10);
            out << val_double;
            out.precision(p);
            break;
        }
        case JsonTypeInt64: {
            out << val_int64_t;
            break;
        }
        case JsonTypeUInt64: {
            out << val_uint64_t;
            break;
        }
        default:
            throw std::invalid_argument("unknown type");
    }
}

std::string JsonObj::str(bool indent) const {
    std::stringstream ss;
    ss.precision(std::numeric_limits<float>::max_digits10);
    write(ss, indent ? 0 : INT64_MIN);
    return ss.str();
}

std::ostream &stim_draw_internal::operator<<(std::ostream &out, const JsonObj &obj) {
    auto precision = out.precision();
    out.precision(std::numeric_limits<float>::max_digits10);
    obj.write(out);
    out.precision(precision);
    return out;
}
