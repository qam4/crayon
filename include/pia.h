#ifndef CRAYON_PIA_H
#define CRAYON_PIA_H

#include "types.h"
#include <cstdint>

namespace crayon {

class InputHandler;
class CassetteInterface;
class LightPen;
class AudioSystem;

struct PIAState {
    uint8_t dra = 0, ddra = 0, cra = 0;
    uint8_t drb = 0, ddrb = 0, crb = 0;
    uint8_t output_latch_a = 0, output_latch_b = 0;
    uint8_t input_pins_a = 0xFF, input_pins_b = 0xFF;
    bool irqa1_flag = false, irqa2_flag = false;
    bool irqb1_flag = false, irqb2_flag = false;
};

class PIA {
public:
    PIA();
    ~PIA() = default;

    void reset();
    uint8_t read_register(uint8_t reg);
    void write_register(uint8_t reg, uint8_t value);

    void set_input_handler(InputHandler* input);
    void set_cassette(CassetteInterface* cass);
    void set_light_pen(LightPen* lp);
    void set_audio(AudioSystem* audio);

    bool irq_active() const;
    bool firq_active() const;

    void signal_vsync();
    void signal_lightpen();

    PIAState get_state() const;
    void set_state(const PIAState& state);

private:
    PIAState state_;
    InputHandler* input_ = nullptr;
    CassetteInterface* cassette_ = nullptr;
    LightPen* light_pen_ = nullptr;
    AudioSystem* audio_ = nullptr;
};

} // namespace crayon

#endif // CRAYON_PIA_H
