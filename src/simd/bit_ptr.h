#ifndef BIT_PTR_H
#define BIT_PTR_H

#include <cstddef>
#include <cstdint>

struct BitPtr {
    uint8_t *byte;
    uint8_t bit_index;

    BitPtr(void *base, size_t offset);
    [[nodiscard]] bool get() const;
    void set(bool new_value);
    void toggle();
    void toggle_if(bool condition);
    void swap(BitPtr &other);
};

struct BitRef {
    uint8_t *byte;
    uint8_t bit_index;

    operator bool() const; // NOLINT(google-explicit-constructor)
    BitRef(void *base, size_t offset);
    BitRef &operator=(bool value);
    BitRef &operator=(const BitRef &value);
    BitRef &operator^=(bool value);
    BitRef &operator&=(bool value);
    BitRef &operator|=(bool value);
    void swap_with(BitRef &other);
};

#endif
