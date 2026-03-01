#ifndef CRAYON_AUDIO_SYSTEM_H
#define CRAYON_AUDIO_SYSTEM_H

#include "types.h"
#include <cstdint>
#include <cstddef>

namespace crayon {

struct AudioState {
    bool buzzer_state = false;
    uint32_t sample_accumulator = 0;
    uint32_t host_sample_rate = 48000;
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem() = default;

    void reset();
    void set_buzzer_bit(bool on);
    void generate_samples(int cpu_cycles);
    void fill_audio_buffer(int16_t* buffer, size_t samples);
    size_t samples_available() const;

    AudioState get_state() const;
    void set_state(const AudioState& state);

private:
    static constexpr size_t RING_BUFFER_SIZE = 4096;
    int16_t ring_buffer_[RING_BUFFER_SIZE] = {};
    size_t write_pos_ = 0;
    size_t read_pos_ = 0;
    AudioState state_;
};

} // namespace crayon

#endif // CRAYON_AUDIO_SYSTEM_H
