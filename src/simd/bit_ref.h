#ifndef BIT_PTR_H
#define BIT_PTR_H

#include <cstddef>
#include <cstdint>

/// A reference to a bit within a byte.
///
/// Conceptually behaves the same as a `bool &`, as opposed to a `bool *`. For example, the `=` operator overwrites the
/// contents of the bit being referenced instead of changing which bit is pointed to.
///
/// This should behave essentially identically to the weird bit references that come out of a `std::vector<bool>`.
struct bit_ref {
    uint8_t *byte;
    uint8_t bit_index;

    /// Construct a bit_ref from a pointer and a bit offset.
    /// The offset can be larger than a word.
    /// Automatically canonicalized so that the offset is less than 8.
    bit_ref(void *base, size_t offset);

    /// Copy assignment.
    bit_ref &operator=(bool value);
    /// Copy assignment.
    bit_ref &operator=(const bit_ref &value);
    /// Xor assignment.
    bit_ref &operator^=(bool value);
    /// Bitwise-and assignment.
    bit_ref &operator&=(bool value);
    /// Bitwise-or assignment.
    bit_ref &operator|=(bool value);
    /// Swap assignment.
    void swap_with(bit_ref other);

    /// Implicit conversion to bool.
    operator bool() const; // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
};

#endif
