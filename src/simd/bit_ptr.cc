#include "bit_ptr.h"

BitPtr::BitPtr(void *base, size_t init_offset) {
    byte = (uint8_t *)base + (init_offset / 8);
    bit_index = init_offset & 7;
}

bool BitPtr::get() const {
    return (*byte >> bit_index) & 1;
}

void BitPtr::set(bool new_value) {
    *byte &= ~((uint8_t)1 << bit_index);
    *byte |= (uint8_t)new_value << bit_index;
}

void BitPtr::toggle() {
    *byte ^= 1 << bit_index;
}

void BitPtr::swap(BitPtr &other) {
    bool b = other.get();
    other.set(get());
    set(b);
}

void BitPtr::toggle_if(bool condition) {
    *byte ^= (uint8_t)condition << bit_index;
}

BitRef::BitRef(void *base, size_t init_offset) : byte((uint8_t *)base + (init_offset / 8)), bit_index(init_offset & 7) {
}

BitRef::operator bool() const {
    return (*byte >> bit_index) & 1;
}

BitRef &BitRef::operator=(bool value) {
    *byte &= ~((uint8_t)1 << bit_index);
    *byte |= (uint8_t)value << bit_index;
    return *this;
}

BitRef &BitRef::operator=(const BitRef &value) {
    *this = (bool)value;
    return *this;
}

BitRef &BitRef::operator^=(bool value) {
    *byte ^= (uint8_t)value << bit_index;
    return *this;
}

BitRef &BitRef::operator&=(bool value) {
    *byte &= (uint8_t)value << bit_index;
    return *this;
}

BitRef &BitRef::operator|=(bool value) {
    *byte |= (uint8_t)value << bit_index;
    return *this;
}

void BitRef::swap_with(BitRef &other) {
    bool b = (bool)other;
    other = (bool)*this;
    *this = b;
}
