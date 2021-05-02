#pragma once

#include <cstdint>
#include <strings.h>
#include <functional>
#include <limits>

namespace Utilities {

template <uint32_t Size>
class BitSet {
    public:

    BitSet() : BitSet(false) {}

    explicit BitSet(bool defaultSet) {
        size_t fillValue;
        if (defaultSet) {
            fillValue = std::numeric_limits<size_t>::max();
        } else {
            fillValue = static_cast<size_t>(0);
        }

        std::fill_n(bitfields, BITFIELD_COUNT, fillValue);
    }

    /**
     * Sets a bit in the set
     */
    inline void set(size_t index) {
        auto bitfieldIndex = index / FIELD_BITS;
        auto bit = index % FIELD_BITS;

        bitfields[bitfieldIndex] |= static_cast<size_t>(1) << bit;
    }

    /**
     * Resets a bit in the set
     */
    inline void reset(size_t index) {
        auto bitfieldIndex = index / FIELD_BITS;
        auto bit = index % FIELD_BITS;

        bitfields[bitfieldIndex] &= ~(static_cast<size_t>(1) << bit);
    }

    /**
     * Find the first set bit in this bitset.
     * @return index + 1 of the set bit, or 0 if none are set.
     */
    inline size_t findFirstSet() const {
        for (size_t bitfieldIndex = 0; bitfieldIndex < BITFIELD_COUNT; ++bitfieldIndex) {
            size_t bitfield = bitfields[bitfieldIndex];
            auto bit = ffsl(bitfield);
            if (bit == 0) {
                continue;
            }

            return bitfieldIndex * FIELD_BITS + bit;
        }

        return 0;
    }

    /**
     * Iterates over each set bit in the set, calling the callback for each.
     * @param callback: A function that takes the index of the set bit and returns nothing.
     */
    inline void forEachSet(const std::function<void(size_t index)> &callback) {
        for (size_t bitfieldIndex = 0; bitfieldIndex < BITFIELD_COUNT; ++bitfieldIndex) {
            size_t bitfield = bitfields[bitfieldIndex];

            while (bitfield) {
                auto bit = ffsl(bitfield);
                auto index = bitfieldIndex * FIELD_BITS + bit - 1;
                
                callback(index);

                bitfield -= static_cast<size_t>(1) << (bit - 1);
            }
        }
    }

    /**
     * Clears all bits in the set
     */
    inline void clear() {
        std::fill_n(bitfields, BITFIELD_COUNT, static_cast<size_t>(0));
    }

    private:
    static const size_t FIELD_BITS = sizeof(size_t) * 8;
    static const size_t BITFIELD_COUNT = ((Size + FIELD_BITS - 1) & ~(FIELD_BITS - 1)) / FIELD_BITS;

    size_t bitfields[BITFIELD_COUNT];
};

}