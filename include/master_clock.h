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
    void tick();

    bool cpu_ready() const;
    bool frame_complete() const;
    void clear_frame_complete();

    uint64_t get_cycle_count() const;
    uint32_t get_current_scanline() const;
    uint32_t get_scanline_cycle() const;

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

} // namespace crayon

#endif // CRAYON_MASTER_CLOCK_H
