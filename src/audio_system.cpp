#include "audio_system.h"
#include <cstring>
#include <algorithm>

namespace crayon {

AudioSystem::AudioSystem() { reset(); }

void AudioSystem::reset() {
    state_ = AudioState{};
    write_pos_ = 0;
    read_pos_ = 0;
    cycle_counter_ = 0;
    std::memset(ring_buffer_, 0, sizeof(ring_buffer_));
}

void AudioSystem::set_buzzer_bit(bool on) {
    // Before changing state, flush accumulated cycles as samples at the OLD state
    flush_cycles();
    if (on != state_.buzzer_state) {
        toggle_count_++;
        cycles_since_toggle_ = 0;
    }
    state_.buzzer_state = on;
}

void AudioSystem::tick(int cpu_cycles) {
    cycle_counter_ += cpu_cycles;
    cycles_since_toggle_ += cpu_cycles;
}

void AudioSystem::generate_samples(int /*cpu_cycles*/) {
    // Flush any remaining accumulated cycles into samples
    flush_cycles();
}

void AudioSystem::flush_cycles() {
    if (cycle_counter_ == 0) return;

    // Convert accumulated CPU cycles to audio samples
    // CPU clock = 1 MHz, sample rate = 48000 Hz
    // samples = cycles * sample_rate / cpu_clock
    // Use fixed-point to avoid drift: accumulate fractional samples
    // 1-bit buzzer: swing between -8192 and +8192 when toggling.
    // After toggling stops, the output decays to 0 to avoid DC offset.
    int16_t sample;
    if (toggle_count_ == 0) {
        sample = 0;  // No toggles ever — silence
    } else if (cycles_since_toggle_ > 50000) {
        // ~50ms without a toggle — buzzer is idle, output silence
        sample = 0;
    } else {
        sample = state_.buzzer_state ? 8192 : -8192;
    }

    // cycles * 48000 / 1000000 = cycles * 48 / 1000 = cycles * 6 / 125
    uint64_t numerator = static_cast<uint64_t>(cycle_counter_) * state_.host_sample_rate
                         + state_.sample_accumulator;
    uint32_t samples_to_generate = static_cast<uint32_t>(numerator / 1000000);
    state_.sample_accumulator = static_cast<uint32_t>(numerator % 1000000);

    for (uint32_t i = 0; i < samples_to_generate; ++i) {
        ring_buffer_[write_pos_ % RING_BUFFER_SIZE] = sample;
        write_pos_++;
    }

    cycle_counter_ = 0;
}

void AudioSystem::fill_audio_buffer(int16_t* buffer, size_t samples) {
    // If ring buffer has too many samples queued, skip ahead to reduce latency
    // Target: keep ~2 frames worth of buffer (1920 samples at 48kHz/50Hz)
    size_t available = (write_pos_ >= read_pos_) ? (write_pos_ - read_pos_) : 0;
    if (available > 2880) {  // More than 3 frames queued — skip to reduce latency
        read_pos_ = write_pos_ - 1920;  // Keep ~2 frames
    }

    for (size_t i = 0; i < samples; ++i) {
        int16_t raw;
        if (read_pos_ < write_pos_) {
            raw = ring_buffer_[read_pos_ % RING_BUFFER_SIZE];
            read_pos_++;
        } else {
            // Underrun: hold last sample instead of silence to avoid clicks
            raw = prev_sample_;
            underrun_count_++;
        }
        // Simple single-pole low-pass filter to reduce harsh square wave harmonics
        prev_sample_ = static_cast<int16_t>(prev_sample_ + ((raw - prev_sample_) * 3 / 20));
        buffer[i] = prev_sample_;
    }
}

size_t AudioSystem::samples_available() const {
    return (write_pos_ >= read_pos_) ? (write_pos_ - read_pos_) : 0;
}

AudioState AudioSystem::get_state() const { return state_; }
void AudioSystem::set_state(const AudioState& state) { state_ = state; }

} // namespace crayon
