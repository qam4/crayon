#ifndef CRAYON_SAVESTATE_H
#define CRAYON_SAVESTATE_H

#include "types.h"
#include "cpu6809.h"
#include "gate_array.h"
#include "memory_system.h"
#include "pia.h"
#include "audio_system.h"
#include "input_handler.h"
#include "light_pen.h"
#include "cassette_interface.h"

namespace crayon {

struct SaveState {
    uint32 version = 0;
    CPU6809State cpu_state;
    GateArrayState gate_array_state;
    MO5MemoryState memory_state;
    PIAState pia_state;
    AudioState audio_state;
    InputState input_state;
    LightPenState light_pen_state;
    CassetteState cassette_state;
    uint64 frame_count = 0;
    uint32 checksum = 0;
};

class SaveStateManager {
public:
    static Result<void> save(const std::string& path, const SaveState& state);
    static Result<SaveState> load(const std::string& path);

private:
    static constexpr uint32 CURRENT_VERSION = 1;
    static uint32 calculate_checksum(const SaveState& state);
    static bool verify_checksum(const SaveState& state);
};

} // namespace crayon

#endif // CRAYON_SAVESTATE_H
