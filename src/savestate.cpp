#include "savestate.h"
#include <fstream>

namespace crayon {

Result<void> SaveStateManager::save(const std::string& path, const SaveState& state) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open file for writing: " + path);
    // TODO: Proper serialization
    file.write(reinterpret_cast<const char*>(&state.version), sizeof(state.version));
    file.write(reinterpret_cast<const char*>(&state.frame_count), sizeof(state.frame_count));
    return Result<void>::ok();
}

Result<SaveState> SaveStateManager::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Result<SaveState>::err("Cannot open file for reading: " + path);
    SaveState state{};
    file.read(reinterpret_cast<char*>(&state.version), sizeof(state.version));
    if (state.version != CURRENT_VERSION)
        return Result<SaveState>::err("Incompatible save state version");
    file.read(reinterpret_cast<char*>(&state.frame_count), sizeof(state.frame_count));
    return Result<SaveState>::ok(state);
}

uint32 SaveStateManager::calculate_checksum(const SaveState& /*state*/) {
    // TODO: Implement proper checksum
    return 0;
}

bool SaveStateManager::verify_checksum(const SaveState& state) {
    return state.checksum == calculate_checksum(state);
}

} // namespace crayon
