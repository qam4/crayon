#include "cassette_interface.h"
#include <fstream>
#include <iterator>

namespace crayon {

CassetteInterface::CassetteInterface() { reset(); }

void CassetteInterface::reset() {
    // Preserve loaded K7 data across reset
    auto saved_data = std::move(state_.k7_data);
    state_ = CassetteState{};
    state_.k7_data = std::move(saved_data);
}

Result<void> CassetteInterface::load_k7(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open K7 file: " + path);
    state_.k7_data.assign((std::istreambuf_iterator<char>(file)), {});
    state_.read_position = 0;
    state_.bit_position = 0;
    return Result<void>::ok();
}

Result<void> CassetteInterface::save_k7(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot write K7 file: " + path);
    file.write(reinterpret_cast<const char*>(state_.record_buffer.data()),
               state_.record_buffer.size());
    return Result<void>::ok();
}

void CassetteInterface::play(uint64_t current_master_cycle) {
    state_.playing = true;
    state_.recording = false;
    state_.play_start_cycle = current_master_cycle;
}

void CassetteInterface::stop() { state_.playing = false; state_.recording = false; }
void CassetteInterface::rewind(uint64_t current_master_cycle) {
    state_.read_position = 0;
    state_.bit_position = 0;
    state_.play_start_cycle = current_master_cycle;
}

void CassetteInterface::update_cycle(uint64_t cycle) {
    state_.current_cycle = cycle;
}

bool CassetteInterface::read_data_bit() {
    // Auto-play: if ROM reads cassette while tape is loaded but not playing, start it
    if (!state_.playing && !state_.k7_data.empty()) {
        state_.playing = true;
        state_.recording = false;
        state_.play_start_cycle = state_.current_cycle;
    }

    if (!state_.playing || state_.k7_data.empty()) return false;

    // K7 format: 1200 baud = 833.33 CPU cycles per bit at 1 MHz
    // Calculate which bit we should be presenting based on elapsed cycles
    static constexpr double CYCLES_PER_BIT = 1000000.0 / 1200.0;  // ~833.33

    uint64_t elapsed = state_.current_cycle - state_.play_start_cycle;
    size_t total_bit = static_cast<size_t>(elapsed / CYCLES_PER_BIT);
    size_t byte_pos = total_bit / 8;
    uint8_t bit_pos = total_bit % 8;

    if (byte_pos >= state_.k7_data.size()) {
        state_.playing = false;
        return false;
    }

    // Track position for save state
    state_.read_position = byte_pos;
    state_.bit_position = bit_pos;

    return (state_.k7_data[byte_pos] >> (7 - bit_pos)) & 1;
}

void CassetteInterface::write_data_bit(bool bit) {
    if (!state_.recording) {
        state_.recording = true;
        state_.playing = false;
    }
    // Pack bits into bytes, MSB first
    if (state_.bit_position == 0) {
        state_.record_buffer.push_back(0);
    }
    if (bit) {
        state_.record_buffer.back() |= (1 << (7 - state_.bit_position));
    }
    state_.bit_position++;
    if (state_.bit_position >= 8) {
        state_.bit_position = 0;
    }
}

bool CassetteInterface::is_playing() const { return state_.playing; }
bool CassetteInterface::is_recording() const { return state_.recording; }

CassetteState CassetteInterface::get_state() const { return state_; }
void CassetteInterface::set_state(const CassetteState& state) { state_ = state; }

} // namespace crayon
