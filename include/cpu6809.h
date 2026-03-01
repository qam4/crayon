#ifndef CRAYON_CPU6809_H
#define CRAYON_CPU6809_H

#include "types.h"
#include <cstdint>

namespace crayon {

class MemorySystem;

struct CPU6809State {
    uint8_t a = 0;
    uint8_t b = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t u = 0;       // User stack pointer
    uint16_t s = 0;       // Hardware stack pointer
    uint16_t pc = 0;
    uint8_t dp = 0;       // Direct page register
    uint8_t cc = 0;       // Condition code register

    // CC flag bit positions
    static constexpr uint8_t CC_C = 0x01;  // Carry
    static constexpr uint8_t CC_V = 0x02;  // Overflow
    static constexpr uint8_t CC_Z = 0x04;  // Zero
    static constexpr uint8_t CC_N = 0x08;  // Negative
    static constexpr uint8_t CC_I = 0x10;  // IRQ mask
    static constexpr uint8_t CC_H = 0x20;  // Half carry
    static constexpr uint8_t CC_F = 0x40;  // FIRQ mask
    static constexpr uint8_t CC_E = 0x80;  // Entire state saved

    uint64_t clock_cycles = 0;
    bool irq_line = false;
    bool firq_line = false;
    bool nmi_pending = false;
    bool nmi_armed = false;
    bool halted = false;
    bool cwai_waiting = false;

    uint16_t d() const { return (static_cast<uint16_t>(a) << 8) | b; }
    void set_d(uint16_t val) { a = val >> 8; b = val & 0xFF; }
};

class CPU6809 {
public:
    CPU6809();
    ~CPU6809() = default;

    void reset();
    uint8_t execute_instruction();  // Returns cycles consumed

    void set_memory_system(MemorySystem* mem);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    void assert_irq(bool active);
    void assert_firq(bool active);
    void assert_nmi();

    CPU6809State get_state() const;
    void set_state(const CPU6809State& state);

    uint16_t get_pc() const { return state_.pc; }
    uint64_t get_cycles() const { return state_.clock_cycles; }

private:
    CPU6809State state_;
    MemorySystem* memory_ = nullptr;
};

} // namespace crayon

#endif // CRAYON_CPU6809_H
