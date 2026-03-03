#include "pia.h"
#include "input_handler.h"
#include "cassette_interface.h"
#include "light_pen.h"
#include "audio_system.h"
#include "memory_system.h"

namespace crayon {

PIA::PIA() { reset(); }

void PIA::reset() { state_ = PIAState{}; }

uint8_t PIA::read_register(uint8_t reg) {
    switch (reg & 0x03) {
        case 0: // Port A data/direction (not used on MO5 — 0xA7C0 is gate array reg)
            if (state_.cra & 0x04) {
                if (cassette_ && cassette_->is_playing()) {
                    bool bit = cassette_->read_data_bit();
                    state_.input_pins_a = (state_.input_pins_a & 0x7F) | (bit ? 0x80 : 0x00);
                }
                uint8_t data = (state_.output_latch_a & state_.ddra) |
                               (state_.input_pins_a & ~state_.ddra);
                state_.irqa1_flag = false;
                state_.irqa2_flag = false;
                return data;
            }
            return state_.ddra;
        case 1: // Control register A
            return state_.cra | (state_.irqa1_flag ? 0x80 : 0) | (state_.irqa2_flag ? 0x40 : 0);
        case 2: // Port B data/direction
            if (state_.crb & 0x04) {
                // MO5 Port B keyboard scanning:
                // The output latch (bits 1-6) selects a key scancode.
                // Bit 7 returns the key state: 0 = pressed, 1 = not pressed.
                // Bit 0 is sound output (active low buzzer).
                // We return the output latch with bit 7 set from keyboard.
                uint8_t key_bit = 0x80;  // default: no key pressed
                if (input_) {
                    key_bit = input_->read_key_state(state_.output_latch_b);
                }
                uint8_t data = (state_.output_latch_b & 0x7F) | key_bit;
                state_.irqb1_flag = false;
                state_.irqb2_flag = false;
                return data;
            }
            return state_.ddrb;
        case 3: // Control register B
            return state_.crb | (state_.irqb1_flag ? 0x80 : 0) | (state_.irqb2_flag ? 0x40 : 0);
    }
    return 0xFF;
}

void PIA::write_register(uint8_t reg, uint8_t value) {
    switch (reg & 0x03) {
        case 0:
            if (state_.cra & 0x04) {
                state_.output_latch_a = value;
                state_.dra = (value & state_.ddra) | (state_.input_pins_a & ~state_.ddra);
                // MO5 Port A bit 0: cassette write output
                if (cassette_ && cassette_->is_recording()) {
                    cassette_->write_data_bit((value & 0x01) != 0);
                }
            } else {
                state_.ddra = value;
            }
            break;
        case 1: state_.cra = value & 0x3F; break;
        case 2:
            if (state_.crb & 0x04) {
                // MO5 Port B write: bits 1-6 = key scancode, bit 0 = sound
                state_.output_latch_b = value & 0x7F;
                state_.drb = state_.output_latch_b;
                // Bit 0 of Port B is the buzzer/sound output on MO5
                if (audio_) audio_->set_buzzer_bit(value & 0x01);
            } else {
                state_.ddrb = value;
            }
            break;
        case 3: state_.crb = value & 0x3F; break;
    }
}

void PIA::set_input_handler(InputHandler* input) { input_ = input; }
void PIA::set_cassette(CassetteInterface* cass) { cassette_ = cass; }
void PIA::set_light_pen(LightPen* lp) { light_pen_ = lp; }
void PIA::set_audio(AudioSystem* audio) { audio_ = audio; }
void PIA::set_memory(MemorySystem* mem) { memory_ = mem; }

bool PIA::irq_active() const {
    return (state_.irqa1_flag && (state_.cra & 0x01)) ||
           (state_.irqa2_flag && (state_.cra & 0x08));
}

bool PIA::firq_active() const {
    return (state_.irqb1_flag && (state_.crb & 0x01)) ||
           (state_.irqb2_flag && (state_.crb & 0x08));
}

void PIA::signal_vsync() { state_.irqb1_flag = true; }
void PIA::signal_lightpen() { state_.irqa1_flag = true; }

void PIA::acknowledge_vsync() {
    // Clear the vsync (CB1) interrupt flag. The ROM's IRQ handler at $F657
    // normally clears this by reading Port B data ($A7C1) at $F691.
    // This method exists as a safety net for edge cases.
    state_.irqb1_flag = false;
}

PIAState PIA::get_state() const { return state_; }
void PIA::set_state(const PIAState& state) { state_ = state; }

} // namespace crayon
