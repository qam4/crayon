#ifndef CRAYON_TYPES_H
#define CRAYON_TYPES_H

#include <cstdint>
#include <string>
#include <optional>

namespace crayon {

// Type aliases
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

// Result type for error handling
template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    
    bool is_ok() const { return value.has_value(); }
    bool is_err() const { return !value.has_value(); }
    
    static Result ok(T val) { Result r; r.value = val; return r; }
    static Result err(const std::string& msg) { Result r; r.error = msg; return r; }
};

template<>
struct Result<void> {
    std::string error;
    
    bool is_ok() const { return error.empty(); }
    bool is_err() const { return !error.empty(); }
    
    static Result ok() { return Result{}; }
    static Result err(const std::string& msg) { Result r; r.error = msg; return r; }
};

// Color structure
struct Color {
    uint8 r, g, b;
    constexpr Color() : r(0), g(0), b(0) {}
    constexpr Color(uint8 red, uint8 green, uint8 blue) : r(red), g(green), b(blue) {}
};

// MO5 display constants
constexpr int DISPLAY_WIDTH = 320;
constexpr int DISPLAY_HEIGHT = 200;
constexpr int TOTAL_SCANLINES = 312;  // PAL

// MO5 16-color fixed palette (EFGJ03L gate array)
constexpr uint32 MO5_PALETTE_RGBA[16] = {
    0x000000FF,  //  0: black
    0xFF0000FF,  //  1: red
    0x00FF00FF,  //  2: green
    0xFFFF00FF,  //  3: yellow
    0x0000FFFF,  //  4: blue
    0xFF00FFFF,  //  5: magenta
    0x00FFFFFF,  //  6: cyan
    0xFFFFFFFF,  //  7: white
    0x808080FF,  //  8: grey
    0xFF8080FF,  //  9: light red (pink)
    0x80FF80FF,  // 10: light green
    0xFFFF80FF,  // 11: light yellow
    0x8080FFFF,  // 12: light blue
    0xFF80FFFF,  // 13: light magenta
    0x80FFFFFF,  // 14: light cyan
    0xFF8000FF   // 15: orange
};

} // namespace crayon

#endif // CRAYON_TYPES_H
