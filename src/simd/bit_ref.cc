#include "bit_ref.h"

bit_ref::bit_ref(void *base, size_t init_offset)
    : byte((uint8_t *)base + (init_offset / 8)), bit_index(init_offset & 7) {}
