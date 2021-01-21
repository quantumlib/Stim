#include "bit_ref.h"

bit_ref::bit_ref(void *base, size_t init_offset) : byte((uint8_t *)base + (init_offset / 8)), bit_index(init_offset & 7) {
}

bit_ref::operator bool() const {
    return (*byte >> bit_index) & 1;
}

bit_ref &bit_ref::operator=(bool value) {
    *byte &= ~((uint8_t)1 << bit_index);
    *byte |= (uint8_t)value << bit_index;
    return *this;
}

bit_ref &bit_ref::operator=(const bit_ref &value) {
    *this = (bool)value;
    return *this;
}

bit_ref &bit_ref::operator^=(bool value) {
    *byte ^= (uint8_t)value << bit_index;
    return *this;
}

bit_ref &bit_ref::operator&=(bool value) {
    *byte &= (uint8_t)value << bit_index;
    return *this;
}

bit_ref &bit_ref::operator|=(bool value) {
    *byte |= (uint8_t)value << bit_index;
    return *this;
}

void bit_ref::swap_with(bit_ref other) {
    bool b = (bool)other;
    other = (bool)*this;
    *this = b;
}
