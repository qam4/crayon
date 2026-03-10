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
    void set_dac_sample(int16_t sample);  // 6-bit DAC from game extension
    void tick(int cpu_cycles);          // Call after each instruction with cycle count
    void generate_samples(int cpu_cycles); // Call at end of frame to flush remaining
    void fill_audio_buffer(int16_t* buffer, size_t samples);
    size_t samples_available() const;
    size_t underrun_count() const { return underrun_count_; }
    size_t toggle_count() const { return toggle_count_; }
    size_t porta_toggle_count() const { return porta_toggle_count_; }
    void increment_porta_toggle() { porta_toggle_count_++; }

    AudioState get_state() const;
    void set_state(const AudioState& state);

private:
    static constexpr size_t RING_BUFFER_SIZE = 16384;
    int16_t ring_buffer_[RING_BUFFER_SIZE] = {};
    size_t write_pos_ = 0;
    size_t read_pos_ = 0;
    uint32_t cycle_counter_ = 0;
    uint32_t cycles_since_toggle_ = 100000;  // Start high = idle
    int16_t prev_sample_ = 0;  // Low-pass filter state
    int16_t dac_sample_ = 0;   // 6-bit DAC output from game extension
    bool dac_active_ = false;  // True once DAC has been written to
    size_t underrun_count_ = 0;
    size_t toggle_count_ = 0;
    size_t porta_toggle_count_ = 0;
    AudioState state_;

    void flush_cycles();
};

} // namespace crayon

#endif // CRAYON_AUDIO_SYSTEM_H
