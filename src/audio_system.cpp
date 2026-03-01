#include "audio_system.h"
#include <cstring>
#include <algorithm>

namespace crayon {

AudioSystem::AudioSystem() { reset(); }

void AudioSystem::reset() {
    state_ = AudioState{};
    write_pos_ = 0;
    read_pos_ = 0;
    std::memset(ring_buffer_, 0, sizeof(ring_buffer_));
}

void AudioSystem::set_buzzer_bit(bool on) { state_.buzzer_state = on; }

void AudioSystem::generate_samples(int cpu_cycles) {
    // MO5 1-bit buzzer: generate square wave samples proportional to CPU cycles
    int16_t sample = state_.buzzer_state ? 8192 : -8192;
    int samples_to_generate = (cpu_cycles * state_.host_sample_rate) / 1000000;
    for (int i = 0; i < samples_to_generate; ++i) {
        ring_buffer_[write_pos_ % RING_BUFFER_SIZE] = sample;
        write_pos_++;
    }
}

void AudioSystem::fill_audio_buffer(int16_t* buffer, size_t samples) {
    for (size_t i = 0; i < samples; ++i) {
        if (read_pos_ < write_pos_) {
            buffer[i] = ring_buffer_[read_pos_ % RING_BUFFER_SIZE];
            read_pos_++;
        } else {
            buffer[i] = 0;
        }
    }
}

size_t AudioSystem::samples_available() const {
    return (write_pos_ >= read_pos_) ? (write_pos_ - read_pos_) : 0;
}

AudioState AudioSystem::get_state() const { return state_; }
void AudioSystem::set_state(const AudioState& state) { state_ = state; }

} // namespace crayon
