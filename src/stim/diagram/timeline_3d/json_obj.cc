#include "stim/diagram/timeline_3d/json_obj.h"
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
        out << (double)num;
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
    ss.precision(std::numeric_limits<double>::max_digits10);
    write(ss, indent ? 0 : INT64_MIN);
    return ss.str();
}

char u6_to_base64_char(uint8_t v) {
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

void stim_draw_internal::write_base64(const char *data, size_t n, std::ostream &out) {
    uint32_t buf = 0;
    size_t bits_in_buf = 0;
    for (size_t k = 0; k < n; k++) {
        buf <<= 8;
        buf |= (uint8_t)data[k];
        bits_in_buf += 8;
        if (bits_in_buf == 24) {
            out << u6_to_base64_char((buf >> 18) & 0x3F);
            out << u6_to_base64_char((buf >> 12) & 0x3F);
            out << u6_to_base64_char((buf >> 6) & 0x3F);
            out << u6_to_base64_char((buf >> 0) & 0x3F);
            bits_in_buf = 0;
            buf = 0;
        }
    }

    if (bits_in_buf) {
        buf <<= (24 - bits_in_buf);
        out << u6_to_base64_char((buf >> 18) & 0x3F);
        out << u6_to_base64_char((buf >> 12) & 0x3F);
        out << (bits_in_buf == 8 ? '=' : u6_to_base64_char((buf >> 6) & 0x3F));
        out << '=';
    }
}
