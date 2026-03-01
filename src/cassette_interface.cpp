#include "cassette_interface.h"
#include <fstream>
#include <iterator>

namespace crayon {

CassetteInterface::CassetteInterface() { reset(); }

void CassetteInterface::reset() { state_ = CassetteState{}; }

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

void CassetteInterface::play() { state_.playing = true; state_.recording = false; }
void CassetteInterface::stop() { state_.playing = false; state_.recording = false; }
void CassetteInterface::rewind() { state_.read_position = 0; state_.bit_position = 0; }

bool CassetteInterface::read_data_bit() {
    if (!state_.playing || state_.read_position >= state_.k7_data.size()) return false;
    bool bit = (state_.k7_data[state_.read_position] >> (7 - state_.bit_position)) & 1;
    state_.bit_position++;
    if (state_.bit_position >= 8) {
        state_.bit_position = 0;
        state_.read_position++;
    }
    return bit;
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
