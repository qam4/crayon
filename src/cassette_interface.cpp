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
    fast_read_pos_ = 0;
    fast_bit_pos_ = 0;
    current_block_ = 0;
    block_byte_pos_ = 0;

    // Parse the raw data into structured blocks
    auto result = parse_k7(state_.k7_data);
    if (result.is_ok()) {
        parsed_file_ = std::move(*result.value);
    }
    // Even if parsing fails, keep raw data for slow mode playback

    return Result<void>::ok();
}

Result<void> CassetteInterface::save_k7(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot write K7 file: " + path);

    // If we have parsed data, serialize from structured blocks
    // Otherwise fall back to raw record buffer
    const auto& data = parsed_file_.blocks.empty()
        ? state_.record_buffer
        : serialize_k7(parsed_file_);

    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
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
    fast_read_pos_ = 0;
    fast_bit_pos_ = 0;
}

void CassetteInterface::update_cycle(uint64_t cycle) {
    state_.current_cycle = cycle;
}

bool CassetteInterface::read_data_bit() {
    // In fast mode, the $F10B/$F181 interception handles all reading.
    // Don't auto-play or advance the slow-mode position.
    if (load_mode_ == CassetteLoadMode::Fast) return false;

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

void CassetteInterface::set_load_mode(CassetteLoadMode mode) { load_mode_ = mode; }
CassetteLoadMode CassetteInterface::get_load_mode() const { return load_mode_; }

bool CassetteInterface::has_data() const { return !parsed_file_.blocks.empty(); }
const K7File& CassetteInterface::get_parsed_file() const { return parsed_file_; }

bool CassetteInterface::try_fast_read_byte(uint8_t& out_byte) {
    if (state_.k7_data.empty()) return false;
    if (fast_read_pos_ >= state_.k7_data.size()) return false;

    out_byte = state_.k7_data[fast_read_pos_++];
    return true;
}

const K7Block* CassetteInterface::try_fast_read_block() {
    if (current_block_ >= parsed_file_.blocks.size()) return nullptr;
    return &parsed_file_.blocks[current_block_++];
}

bool CassetteInterface::try_fast_read_bit(bool& out_bit) {
    if (state_.k7_data.empty()) return false;
    if (fast_read_pos_ >= state_.k7_data.size()) return false;

    out_bit = (state_.k7_data[fast_read_pos_] >> (7 - fast_bit_pos_)) & 1;
    fast_bit_pos_++;
    if (fast_bit_pos_ >= 8) {
        fast_bit_pos_ = 0;
        fast_read_pos_++;
    }
    return true;
}

Result<K7File> CassetteInterface::parse_k7(const std::vector<uint8_t>& raw_data) {
    K7File file{};
    size_t pos = 0;
    const size_t size = raw_data.size();

    if (size == 0) return Result<K7File>::err("Empty K7 data");

    while (pos < size) {
        // Skip leader bytes (0x01)
        while (pos < size && raw_data[pos] == 0x01) {
            ++pos;
        }

        if (pos >= size) break;

        // Expect sync byte 0x3C
        if (raw_data[pos] != 0x3C) {
            // Skip unknown bytes until we find a leader or sync
            ++pos;
            continue;
        }
        ++pos; // consume 0x3C sync byte

        // Skip second sync byte 0x5A if present (standard MO5 cassette format)
        if (pos < size && raw_data[pos] == 0x5A) {
            ++pos;
        }

        // Need at least type + length bytes
        if (pos + 2 > size) {
            return Result<K7File>::err("Truncated block at offset " + std::to_string(pos));
        }

        K7Block block{};
        block.type = raw_data[pos++];
        uint8_t length = raw_data[pos++];

        // Need length bytes + 1 checksum byte
        if (pos + length + 1 > size) {
            return Result<K7File>::err("Truncated block data at offset " + std::to_string(pos));
        }

        block.data.assign(raw_data.begin() + pos, raw_data.begin() + pos + length);
        pos += length;

        block.checksum = raw_data[pos++];

        // Validate checksum: sum of type + length + all data bytes, & 0xFF
        uint8_t computed = block.type + length;
        for (uint8_t b : block.data) {
            computed += b;
        }
        computed &= 0xFF;

        if (computed != block.checksum) {
            return Result<K7File>::err(
                "Checksum mismatch in block at offset " + std::to_string(pos) +
                ": expected 0x" + std::to_string(block.checksum) +
                ", got 0x" + std::to_string(computed));
        }

        // Parse header block metadata
        if (block.type == 0x00 && block.data.size() >= 14) {
            // Filename: first 8 bytes, trim trailing spaces
            file.filename = std::string(block.data.begin(), block.data.begin() + 8);
            while (!file.filename.empty() && file.filename.back() == ' ') {
                file.filename.pop_back();
            }
            file.file_type = block.data[8];
            file.mode = block.data[9];
            file.start_address = (static_cast<uint16_t>(block.data[10]) << 8) | block.data[11];
            file.exec_address = (static_cast<uint16_t>(block.data[12]) << 8) | block.data[13];
        }

        file.blocks.push_back(std::move(block));

        // Stop after EOF block
        if (file.blocks.back().type == 0xFF) break;
    }

    if (file.blocks.empty()) {
        return Result<K7File>::err("No valid blocks found in K7 data");
    }

    return Result<K7File>::ok(std::move(file));
}

std::vector<uint8_t> CassetteInterface::serialize_k7(const K7File& file) {
    std::vector<uint8_t> out;

    for (const auto& block : file.blocks) {
        // Leader: 16 bytes of 0x01
        for (int i = 0; i < 16; ++i) {
            out.push_back(0x01);
        }

        // Sync bytes
        out.push_back(0x3C);
        out.push_back(0x5A);

        // Block type
        out.push_back(block.type);

        // Length
        auto length = static_cast<uint8_t>(block.data.size());
        out.push_back(length);

        // Data payload
        out.insert(out.end(), block.data.begin(), block.data.end());

        // Checksum: sum of type + length + all data bytes, & 0xFF
        uint8_t checksum = block.type + length;
        for (uint8_t b : block.data) {
            checksum += b;
        }
        checksum &= 0xFF;
        out.push_back(checksum);
    }

    return out;
}

CassetteState CassetteInterface::get_state() const { return state_; }
void CassetteInterface::set_state(const CassetteState& state) { state_ = state; }

} // namespace crayon
