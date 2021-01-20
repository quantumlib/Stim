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

void bit_ref::swap_with(bit_ref &other) {
    bool b = (bool)other;
    other = (bool)*this;
    *this = b;
}

const bit_ref bit_ref::const_ref(bool value) {
    return bit_ref(&CONST_BIT_REF_VALUE, value);
}

uint8_t CONST_BIT_REF_VALUE = 0b10;
