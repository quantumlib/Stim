#include "stim/diagram/json_obj.h"

#include <limits>

using namespace stim;
using namespace stim_draw_internal;

JsonObj::JsonObj(bool boolean) : boolean(boolean), type(4) {
}
JsonObj::JsonObj(int num) : num(num), type(0) {
}
JsonObj::JsonObj(size_t num) : num(num), type(0) {
}
JsonObj::JsonObj(float num) : num(num), type(0) {
}
JsonObj::JsonObj(double num) : double_num(num), type(11) {
}

JsonObj::JsonObj(std::string text) : text(text), type(1) {
}

JsonObj::JsonObj(const char *text) : text(text), type(1) {
}

JsonObj::JsonObj(std::map<std::string, JsonObj> map) : map(map), type(2) {
}

JsonObj::JsonObj(std::vector<JsonObj> arr) : arr(arr), type(3) {
}

void JsonObj::clear() {
    auto old_type = type;
    if (old_type == 1) {
        text.clear();
    } else if (old_type == 2) {
        map.clear();
    } else if (old_type == 3) {
        arr.clear();
    }
    type = 0;
    num = 0;
    double_num = 0;
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
    if (type == 0) {
        out << num;
    } else if (type == 11) {
        auto p = out.precision();
        out.precision(std::numeric_limits<double>::digits10);
        out << double_num;
        out.precision(p);
    } else if (type == 1) {
        write_str(text, out);
    } else if (type == 2) {
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
    } else if (type == 3) {
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
    } else if (type == 4) {
        out << (boolean ? "true" : "false");
    } else {
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
