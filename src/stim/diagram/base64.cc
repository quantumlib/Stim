#include "stim/diagram/base64.h"

using namespace stim_draw_internal;

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

void stim_draw_internal::write_data_as_base64_to(std::string_view data, std::ostream &out) {
    uint32_t buf = 0;
    size_t bits_in_buf = 0;
    for (char c : data) {
        buf <<= 8;
        buf |= (uint8_t)c;
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
