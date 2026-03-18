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

} // namespace crayon
