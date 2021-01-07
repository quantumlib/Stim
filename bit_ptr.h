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

#endif
