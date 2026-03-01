#ifndef CRAYON_UTILS_H
#define CRAYON_UTILS_H

#include "types.h"

namespace crayon {
namespace utils {

inline bool get_bit(uint8 value, int bit) { return (value >> bit) & 1; }
inline uint8 set_bit(uint8 value, int bit) { return value | (1 << bit); }
inline uint8 clear_bit(uint8 value, int bit) { return value & ~(1 << bit); }
inline uint8 toggle_bit(uint8 value, int bit) { return value ^ (1 << bit); }
inline uint8 write_bit(uint8 value, int bit, bool state) {
    return state ? set_bit(value, bit) : clear_bit(value, bit);
}

inline uint16 pack_bytes(uint8 high, uint8 low) {
    return (static_cast<uint16>(high) << 8) | low;
}
inline uint8 high_byte(uint16 value) { return static_cast<uint8>(value >> 8); }
inline uint8 low_byte(uint16 value) { return static_cast<uint8>(value & 0xFF); }

inline uint32 calculate_checksum(const uint8* data, size_t length) {
    uint32 checksum = 0;
    for (size_t i = 0; i < length; ++i) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum ^= data[i];
    }
    return checksum;
}

} // namespace utils
} // namespace crayon

#endif // CRAYON_UTILS_H
