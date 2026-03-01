#include "master_clock.h"

namespace crayon {

MasterClock::MasterClock() { reset(); }

void MasterClock::reset() {
    total_cycles_ = 0;
    frame_cycle_ = 0;
    scanline_ = 0;
    scanline_cycle_ = 0;
    frame_complete_ = false;
}

void MasterClock::tick() {
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

bool MasterClock::cpu_ready() const { return true; }
bool MasterClock::frame_complete() const { return frame_complete_; }
void MasterClock::clear_frame_complete() { frame_complete_ = false; }

uint64_t MasterClock::get_cycle_count() const { return total_cycles_; }
uint32_t MasterClock::get_current_scanline() const { return scanline_; }
uint32_t MasterClock::get_scanline_cycle() const { return scanline_cycle_; }

} // namespace crayon
