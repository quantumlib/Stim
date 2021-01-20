#ifndef BIT_PTR_H
#define BIT_PTR_H

#include <cstddef>
#include <cstdint>

struct bit_ref {
    uint8_t *byte;
    uint8_t bit_index;

    operator bool() const; // NOLINT(google-explicit-constructor)
    bit_ref(void *base, size_t offset);
    bit_ref &operator=(bool value);
    bit_ref &operator=(const bit_ref &value);
    bit_ref &operator^=(bool value);
    bit_ref &operator&=(bool value);
    bit_ref &operator|=(bool value);
    void swap_with(bit_ref &other);
};

#endif
