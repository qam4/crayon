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

    // Instruction fetch helpers
    uint8_t fetch();            // Read byte at PC, increment PC
    uint16_t fetch16();         // Read 16-bit big-endian at PC, increment PC by 2

    // Opcode dispatch by page
    uint8_t execute_page1(uint8_t opcode);  // Main page opcodes 0x00-0xFF
    uint8_t execute_page2(uint8_t opcode);  // Page 2 (0x10 prefix) opcodes
    uint8_t execute_page3(uint8_t opcode);  // Page 3 (0x11 prefix) opcodes

    // Addressing mode helpers
    uint16_t addr_direct();                         // DP-relative: (DP << 8) | fetch()
    uint16_t addr_extended();                       // 16-bit absolute: fetch16()
    uint16_t addr_indexed(uint8_t& extra_cycles);   // Complex indexed addressing

    // Indexed addressing helper: returns reference to X, Y, U, or S based on register select bits
    uint16_t& index_register(uint8_t reg_bits);

    // --- 8-bit ALU helpers (set CC flags, return result) ---
    uint8_t op_add8(uint8_t a, uint8_t b, bool carry_in = false);
    uint8_t op_sub8(uint8_t a, uint8_t b, bool carry_in = false);
    void    op_cmp8(uint8_t a, uint8_t b);
    uint8_t op_and8(uint8_t a, uint8_t b);
    uint8_t op_or8(uint8_t a, uint8_t b);
    uint8_t op_eor8(uint8_t a, uint8_t b);
    void    op_ld8(uint8_t& reg, uint8_t val);
    void    op_st8(uint16_t addr, uint8_t val);
    uint8_t op_neg8(uint8_t val);
    uint8_t op_com8(uint8_t val);
    uint8_t op_inc8(uint8_t val);
    uint8_t op_dec8(uint8_t val);
    void    op_tst8(uint8_t val);
    uint8_t op_clr8();

    // --- Shift/rotate helpers ---
    uint8_t op_lsr8(uint8_t val);
    uint8_t op_asr8(uint8_t val);
    uint8_t op_asl8(uint8_t val);
    uint8_t op_ror8(uint8_t val);
    uint8_t op_rol8(uint8_t val);

    // --- 16-bit ALU helpers ---
    uint16_t op_add16(uint16_t a, uint16_t b);
    uint16_t op_sub16(uint16_t a, uint16_t b);
    void     op_cmp16(uint16_t a, uint16_t b);
    void     op_ld16(uint16_t& reg, uint16_t val);
    void     op_st16(uint16_t addr, uint16_t val);
    void     set_nz16(uint16_t val);

    // --- 16-bit memory access ---
    uint16_t read16(uint16_t address);

    // --- Register transfer helpers ---
    uint16_t get_register(uint8_t code);
    void     set_register(uint8_t code, uint16_t val);

    // --- Interrupt helpers ---
    void push_entire_state();   // Push CC,A,B,DP,X,Y,U,PC to S stack (set E flag first)
    void push_firq_state();     // Push CC,PC to S stack (clear E flag first)
    uint8_t check_interrupts(); // Check and handle pending interrupts, return extra cycles

    // --- CC flag helpers ---
    void set_flag(uint8_t flag, bool val);
    bool get_flag(uint8_t flag) const;
    void set_nz8(uint8_t val);
};

} // namespace crayon

#endif // CRAYON_CPU6809_H
