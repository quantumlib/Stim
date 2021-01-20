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

struct bit_ref {
    uint8_t *byte;
    uint8_t bit_index;

    operator bool() const; // NOLINT(google-explicit-constructor)
    static const bit_ref const_ref(bool value);
    bit_ref(void *base, size_t offset);
    bit_ref &operator=(bool value);
    bit_ref &operator=(const bit_ref &value);
    bit_ref &operator^=(bool value);
    bit_ref &operator&=(bool value);
    bit_ref &operator|=(bool value);
    void swap_with(bit_ref &other);
};

extern uint8_t CONST_BIT_REF_VALUE;

#endif
