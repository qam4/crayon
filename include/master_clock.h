#ifndef CRAYON_MASTER_CLOCK_H
#define CRAYON_MASTER_CLOCK_H

#include "types.h"
#include <cstdint>

namespace crayon {

class MasterClock {
public:
    MasterClock();
    ~MasterClock() = default;

    void reset();
    inline void tick();

    inline bool cpu_ready() const;
    inline bool frame_complete() const;
    inline void clear_frame_complete();

    inline uint64_t get_cycle_count() const;
    inline uint32_t get_current_scanline() const;
    inline uint32_t get_scanline_cycle() const;

    static constexpr uint32_t CPU_CLOCK_HZ = 1000000;
    static constexpr uint32_t FRAME_RATE_HZ = 50;
    static constexpr uint32_t CYCLES_PER_FRAME = 20000;
    static constexpr uint32_t SCANLINES_PER_FRAME = 312;
    static constexpr uint32_t VISIBLE_SCANLINES = 200;
    static constexpr uint32_t CYCLES_PER_SCANLINE = 64;

private:
    uint64_t total_cycles_ = 0;
    uint32_t frame_cycle_ = 0;
    uint32_t scanline_ = 0;
    uint32_t scanline_cycle_ = 0;
    bool frame_complete_ = false;
};

inline void MasterClock::tick() {
    total_cycles_++;
    frame_cycle_++;
    scanline_cycle_++;

    if (scanline_cycle_ >= CYCLES_PER_SCANLINE) {
        scanline_cycle_ = 0;
        scanline_++;
    }

    if (frame_cycle_ >= CYCLES_PER_FRAME) {
        frame_cycle_ = 0;
        scanline_ = 0;
        frame_complete_ = true;
    }
}

inline bool MasterClock::cpu_ready() const { return true; }
inline bool MasterClock::frame_complete() const { return frame_complete_; }
inline void MasterClock::clear_frame_complete() { frame_complete_ = false; }
inline uint64_t MasterClock::get_cycle_count() const { return total_cycles_; }
inline uint32_t MasterClock::get_current_scanline() const { return scanline_; }
inline uint32_t MasterClock::get_scanline_cycle() const { return scanline_cycle_; }

} // namespace crayon

#endif // CRAYON_MASTER_CLOCK_H
