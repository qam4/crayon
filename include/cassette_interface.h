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
};

class CassetteInterface {
public:
    CassetteInterface();
    ~CassetteInterface() = default;

    Result<void> load_k7(const std::string& path);
    Result<void> save_k7(const std::string& path);

    void reset();
    void play();
    void stop();
    void rewind();

    bool read_data_bit();
    void write_data_bit(bool bit);

    bool is_playing() const;
    bool is_recording() const;

    CassetteState get_state() const;
    void set_state(const CassetteState& state);

private:
    CassetteState state_;
};

} // namespace crayon

#endif // CRAYON_CASSETTE_INTERFACE_H
