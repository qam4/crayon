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

MasterClock::State MasterClock::get_state() const {
    return {total_cycles_, frame_cycle_, scanline_, scanline_cycle_, frame_complete_};
}

void MasterClock::set_state(const State& s) {
    total_cycles_ = s.total_cycles;
    frame_cycle_ = s.frame_cycle;
    scanline_ = s.scanline;
    scanline_cycle_ = s.scanline_cycle;
    frame_complete_ = s.frame_complete;
}

} // namespace crayon
