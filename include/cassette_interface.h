#ifndef CRAYON_CASSETTE_INTERFACE_H
#define CRAYON_CASSETTE_INTERFACE_H

#include "types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace crayon {

struct CassetteState {
    std::vector<uint8_t> k7_data;
    size_t read_position = 0;
    uint8_t bit_position = 0;
    bool playing = false;
    bool recording = false;
    std::vector<uint8_t> record_buffer;
    uint64_t play_start_cycle = 0;   // CPU cycle when play() was called
    uint64_t current_cycle = 0;      // Updated each frame by emulator
};

enum class CassetteLoadMode {
    Fast,   // Direct data injection (default)
    Slow    // Real-time 1200 baud audio simulation
};

class CassetteInterface {
public:
    CassetteInterface();
    ~CassetteInterface() = default;

    Result<void> load_k7(const std::string& path);
    Result<void> save_k7(const std::string& path);

    void reset();
    void play(uint64_t current_master_cycle);
    void stop();
    void rewind(uint64_t current_master_cycle);

    bool read_data_bit();
    void write_data_bit(bool bit);
    void update_cycle(uint64_t cycle);  // Call each instruction to track timing

    bool is_playing() const;
    bool is_recording() const;

    void set_load_mode(CassetteLoadMode mode);
    CassetteLoadMode get_load_mode() const;

    CassetteState get_state() const;
    void set_state(const CassetteState& state);

private:
    CassetteState state_;
    CassetteLoadMode load_mode_ = CassetteLoadMode::Fast;
};

} // namespace crayon

#endif // CRAYON_CASSETTE_INTERFACE_H
