#pragma once

#include <cstdint>

namespace Utilities {
    
class LehmerRandom {
    public:
    void seed(uint32_t seed) {
        value = seed;
    }

    uint32_t random() {
		value += 0xe120fc15;
		uint64_t tmp;
		tmp = (uint64_t)value * 0x4a39b70d;
		uint32_t m1 = (tmp >> 32) ^ tmp;
		tmp = (uint64_t)m1 * 0x12fad5c9;
		uint32_t m2 = (tmp >> 32) ^ tmp;
		return m2;
	}

    private:
    uint32_t value;
};

}