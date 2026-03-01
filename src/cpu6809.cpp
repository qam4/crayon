#include "cpu6809.h"
#include "memory_system.h"

namespace crayon {

CPU6809::CPU6809() { reset(); }

void CPU6809::reset() {
    state_ = CPU6809State{};
    if (memory_) {
        uint8_t hi = read(0xFFFE);
        uint8_t lo = read(0xFFFF);
        state_.pc = (static_cast<uint16_t>(hi) << 8) | lo;
    }
    state_.cc = CPU6809State::CC_I | CPU6809State::CC_F;
}

uint8_t CPU6809::fetch() {
    return read(state_.pc++);
}

uint16_t CPU6809::fetch16() {
    uint8_t hi = fetch();
    uint8_t lo = fetch();
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

// --- CC flag helpers ---

void CPU6809::set_flag(uint8_t flag, bool val) {
    if (val) state_.cc |= flag;
    else     state_.cc &= ~flag;
}

bool CPU6809::get_flag(uint8_t flag) const {
    return (state_.cc & flag) != 0;
}

void CPU6809::set_nz8(uint8_t val) {
    set_flag(CPU6809State::CC_N, (val & 0x80) != 0);
    set_flag(CPU6809State::CC_Z, val == 0);
}

// --- 8-bit ALU helpers ---

uint8_t CPU6809::op_add8(uint8_t a, uint8_t b, bool carry_in) {
    uint8_t c = carry_in ? 1 : 0;
    uint16_t sum = static_cast<uint16_t>(a) + b + c;
    uint8_t result = static_cast<uint8_t>(sum);
    set_nz8(result);
    set_flag(CPU6809State::CC_C, sum > 0xFF);
    set_flag(CPU6809State::CC_V, ((a ^ result) & (b ^ result) & 0x80) != 0);
    set_flag(CPU6809State::CC_H, ((a ^ b ^ result) & 0x10) != 0);
    return result;
}

uint8_t CPU6809::op_sub8(uint8_t a, uint8_t b, bool carry_in) {
    uint8_t c = carry_in ? 1 : 0;
    uint16_t diff = static_cast<uint16_t>(a) - b - c;
    uint8_t result = static_cast<uint8_t>(diff);
    set_nz8(result);
    set_flag(CPU6809State::CC_C, diff > 0xFF);  // borrow
    set_flag(CPU6809State::CC_V, ((a ^ b) & (a ^ result) & 0x80) != 0);
    set_flag(CPU6809State::CC_H, ((a ^ b ^ result) & 0x10) != 0);
    return result;
}

void CPU6809::op_cmp8(uint8_t a, uint8_t b) {
    op_sub8(a, b, false);  // discard result, only flags matter
}

uint8_t CPU6809::op_and8(uint8_t a, uint8_t b) {
    uint8_t result = a & b;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, false);
    return result;
}

uint8_t CPU6809::op_or8(uint8_t a, uint8_t b) {
    uint8_t result = a | b;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, false);
    return result;
}

uint8_t CPU6809::op_eor8(uint8_t a, uint8_t b) {
    uint8_t result = a ^ b;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, false);
    return result;
}

void CPU6809::op_ld8(uint8_t& reg, uint8_t val) {
    reg = val;
    set_nz8(val);
    set_flag(CPU6809State::CC_V, false);
}

void CPU6809::op_st8(uint16_t addr, uint8_t val) {
    write(addr, val);
    set_nz8(val);
    set_flag(CPU6809State::CC_V, false);
}

uint8_t CPU6809::op_neg8(uint8_t val) {
    uint8_t result = static_cast<uint8_t>(0 - val);
    set_nz8(result);
    set_flag(CPU6809State::CC_V, result == 0x80);
    set_flag(CPU6809State::CC_C, result != 0);
    return result;
}

uint8_t CPU6809::op_com8(uint8_t val) {
    uint8_t result = ~val;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, false);
    set_flag(CPU6809State::CC_C, true);
    return result;
}

uint8_t CPU6809::op_inc8(uint8_t val) {
    uint8_t result = val + 1;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, val == 0x7F);
    // C unchanged
    return result;
}

uint8_t CPU6809::op_dec8(uint8_t val) {
    uint8_t result = val - 1;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, val == 0x80);
    // C unchanged
    return result;
}

void CPU6809::op_tst8(uint8_t val) {
    set_nz8(val);
    set_flag(CPU6809State::CC_V, false);
    // C unchanged
}

uint8_t CPU6809::op_clr8() {
    set_flag(CPU6809State::CC_N, false);
    set_flag(CPU6809State::CC_Z, true);
    set_flag(CPU6809State::CC_V, false);
    set_flag(CPU6809State::CC_C, false);
    return 0;
}

// --- Shift/rotate helpers ---

uint8_t CPU6809::op_lsr8(uint8_t val) {
    set_flag(CPU6809State::CC_C, (val & 0x01) != 0);
    uint8_t result = val >> 1;
    set_flag(CPU6809State::CC_N, false);
    set_flag(CPU6809State::CC_Z, result == 0);
    return result;
}

uint8_t CPU6809::op_asr8(uint8_t val) {
    set_flag(CPU6809State::CC_C, (val & 0x01) != 0);
    uint8_t result = (val >> 1) | (val & 0x80);  // preserve sign bit
    set_nz8(result);
    return result;
}

uint8_t CPU6809::op_asl8(uint8_t val) {
    set_flag(CPU6809State::CC_C, (val & 0x80) != 0);
    uint8_t result = val << 1;
    set_nz8(result);
    set_flag(CPU6809State::CC_V, ((val ^ result) & 0x80) != 0);
    set_flag(CPU6809State::CC_H, ((val ^ result) & 0x10) != 0);
    return result;
}

uint8_t CPU6809::op_ror8(uint8_t val) {
    bool old_carry = get_flag(CPU6809State::CC_C);
    set_flag(CPU6809State::CC_C, (val & 0x01) != 0);
    uint8_t result = (val >> 1) | (old_carry ? 0x80 : 0x00);
    set_nz8(result);
    return result;
}

uint8_t CPU6809::op_rol8(uint8_t val) {
    bool old_carry = get_flag(CPU6809State::CC_C);
    set_flag(CPU6809State::CC_C, (val & 0x80) != 0);
    uint8_t result = (val << 1) | (old_carry ? 0x01 : 0x00);
    set_nz8(result);
    set_flag(CPU6809State::CC_V, ((val ^ result) & 0x80) != 0);
    return result;
}

// --- 16-bit helpers ---

void CPU6809::set_nz16(uint16_t val) {
    set_flag(CPU6809State::CC_N, (val & 0x8000) != 0);
    set_flag(CPU6809State::CC_Z, val == 0);
}

uint16_t CPU6809::op_add16(uint16_t a, uint16_t b) {
    uint32_t sum = static_cast<uint32_t>(a) + b;
    uint16_t result = static_cast<uint16_t>(sum);
    set_nz16(result);
    set_flag(CPU6809State::CC_C, sum > 0xFFFF);
    set_flag(CPU6809State::CC_V, ((a ^ result) & (b ^ result) & 0x8000) != 0);
    return result;
}

uint16_t CPU6809::op_sub16(uint16_t a, uint16_t b) {
    uint32_t diff = static_cast<uint32_t>(a) - b;
    uint16_t result = static_cast<uint16_t>(diff);
    set_nz16(result);
    set_flag(CPU6809State::CC_C, diff > 0xFFFF);  // borrow
    set_flag(CPU6809State::CC_V, ((a ^ b) & (a ^ result) & 0x8000) != 0);
    return result;
}

void CPU6809::op_cmp16(uint16_t a, uint16_t b) {
    op_sub16(a, b);  // discard result, only flags matter
}

void CPU6809::op_ld16(uint16_t& reg, uint16_t val) {
    reg = val;
    set_nz16(val);
    set_flag(CPU6809State::CC_V, false);
}

void CPU6809::op_st16(uint16_t addr, uint16_t val) {
    write(addr, static_cast<uint8_t>(val >> 8));
    write(addr + 1, static_cast<uint8_t>(val & 0xFF));
    set_nz16(val);
    set_flag(CPU6809State::CC_V, false);
}

uint16_t CPU6809::read16(uint16_t address) {
    uint8_t hi = read(address);
    uint8_t lo = read(address + 1);
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

// --- Register transfer helpers ---

uint16_t CPU6809::get_register(uint8_t code) {
    switch (code & 0x0F) {
        case 0x00: return state_.d();
        case 0x01: return state_.x;
        case 0x02: return state_.y;
        case 0x03: return state_.u;
        case 0x04: return state_.s;
        case 0x05: return state_.pc;
        case 0x08: return 0xFF00 | state_.a;
        case 0x09: return 0xFF00 | state_.b;
        case 0x0A: return 0xFF00 | state_.cc;
        case 0x0B: return 0xFF00 | state_.dp;
        default:   return 0xFFFF;
    }
}

void CPU6809::set_register(uint8_t code, uint16_t val) {
    switch (code & 0x0F) {
        case 0x00: state_.set_d(val); break;
        case 0x01: state_.x = val; break;
        case 0x02: state_.y = val; break;
        case 0x03: state_.u = val; break;
        case 0x04: state_.s = val; break;
        case 0x05: state_.pc = val; break;
        case 0x08: state_.a = static_cast<uint8_t>(val); break;
        case 0x09: state_.b = static_cast<uint8_t>(val); break;
        case 0x0A: state_.cc = static_cast<uint8_t>(val); break;
        case 0x0B: state_.dp = static_cast<uint8_t>(val); break;
        default: break;
    }
}

// --- Addressing mode helpers ---

uint16_t CPU6809::addr_direct() {
    return (static_cast<uint16_t>(state_.dp) << 8) | fetch();
}

uint16_t CPU6809::addr_extended() {
    return fetch16();
}

uint16_t& CPU6809::index_register(uint8_t reg_bits) {
    switch (reg_bits & 0x03) {
        case 0x00: return state_.x;
        case 0x01: return state_.y;
        case 0x02: return state_.u;
        case 0x03: return state_.s;
        default:   return state_.x;
    }
}

uint16_t CPU6809::addr_indexed(uint8_t& extra_cycles) {
    uint8_t postbyte = fetch();
    extra_cycles = 0;

    if ((postbyte & 0x80) == 0) {
        uint16_t& reg = index_register((postbyte >> 5) & 0x03);
        int8_t offset = static_cast<int8_t>((postbyte & 0x1F) | ((postbyte & 0x10) ? 0xE0 : 0x00));
        extra_cycles = 1;
        return reg + static_cast<int16_t>(offset);
    }

    uint8_t reg_select = (postbyte >> 5) & 0x03;
    bool indirect = (postbyte & 0x10) != 0;
    uint8_t mode = postbyte & 0x0F;

    uint16_t ea = 0;

    switch (mode) {
        case 0x00: {
            uint16_t& reg = index_register(reg_select);
            ea = reg;
            reg += 1;
            extra_cycles = 2;
            return ea;
        }
        case 0x01: {
            uint16_t& reg = index_register(reg_select);
            ea = reg;
            reg += 2;
            extra_cycles = 3;
            break;
        }
        case 0x02: {
            uint16_t& reg = index_register(reg_select);
            reg -= 1;
            ea = reg;
            extra_cycles = 2;
            return ea;
        }
        case 0x03: {
            uint16_t& reg = index_register(reg_select);
            reg -= 2;
            ea = reg;
            extra_cycles = 3;
            break;
        }
        case 0x04: {
            ea = index_register(reg_select);
            extra_cycles = 0;
            break;
        }
        case 0x05: {
            ea = index_register(reg_select) + static_cast<int16_t>(static_cast<int8_t>(state_.b));
            extra_cycles = 1;
            break;
        }
        case 0x06: {
            ea = index_register(reg_select) + static_cast<int16_t>(static_cast<int8_t>(state_.a));
            extra_cycles = 1;
            break;
        }
        case 0x07: {
            extra_cycles = 0;
            return 0;
        }
        case 0x08: {
            int8_t offset = static_cast<int8_t>(fetch());
            ea = index_register(reg_select) + static_cast<int16_t>(offset);
            extra_cycles = 1;
            break;
        }
        case 0x09: {
            int16_t offset = static_cast<int16_t>(fetch16());
            ea = index_register(reg_select) + offset;
            extra_cycles = 4;
            break;
        }
        case 0x0A: {
            extra_cycles = 0;
            return 0;
        }
        case 0x0B: {
            ea = index_register(reg_select) + state_.d();
            extra_cycles = 4;
            break;
        }
        case 0x0C: {
            int8_t offset = static_cast<int8_t>(fetch());
            ea = state_.pc + static_cast<int16_t>(offset);
            extra_cycles = 1;
            break;
        }
        case 0x0D: {
            int16_t offset = static_cast<int16_t>(fetch16());
            ea = state_.pc + offset;
            extra_cycles = 5;
            break;
        }
        case 0x0E: {
            extra_cycles = 0;
            return 0;
        }
        case 0x0F: {
            ea = fetch16();
            uint8_t hi = read(ea);
            uint8_t lo = read(ea + 1);
            extra_cycles = 5;
            return (static_cast<uint16_t>(hi) << 8) | lo;
        }
        default:
            return 0;
    }

    if (indirect) {
        uint8_t hi = read(ea);
        uint8_t lo = read(ea + 1);
        ea = (static_cast<uint16_t>(hi) << 8) | lo;
        extra_cycles += 3;
    }

    return ea;
}

// --- Interrupt helpers ---

void CPU6809::push_entire_state() {
    set_flag(CPU6809State::CC_E, true);
    write(--state_.s, state_.pc & 0xFF);
    write(--state_.s, state_.pc >> 8);
    write(--state_.s, state_.u & 0xFF);
    write(--state_.s, state_.u >> 8);
    write(--state_.s, state_.y & 0xFF);
    write(--state_.s, state_.y >> 8);
    write(--state_.s, state_.x & 0xFF);
    write(--state_.s, state_.x >> 8);
    write(--state_.s, state_.dp);
    write(--state_.s, state_.b);
    write(--state_.s, state_.a);
    write(--state_.s, state_.cc);
}

void CPU6809::push_firq_state() {
    set_flag(CPU6809State::CC_E, false);
    write(--state_.s, state_.pc & 0xFF);
    write(--state_.s, state_.pc >> 8);
    write(--state_.s, state_.cc);
}

uint8_t CPU6809::check_interrupts() {
    uint8_t cycles = 0;

    // NMI — edge-triggered, highest priority, only if armed
    if (state_.nmi_pending && state_.nmi_armed) {
        state_.nmi_pending = false;
        if (!state_.cwai_waiting) {
            push_entire_state();
            cycles += 19;
        } else {
            state_.cwai_waiting = false;
        }
        state_.halted = false;
        set_flag(CPU6809State::CC_I, true);
        set_flag(CPU6809State::CC_F, true);
        state_.pc = read16(0xFFFC);
        return cycles;
    }

    // FIRQ — level-triggered, if F flag clear
    if (state_.firq_line && !get_flag(CPU6809State::CC_F)) {
        if (!state_.cwai_waiting) {
            push_firq_state();
            cycles += 10;
        } else {
            state_.cwai_waiting = false;
        }
        state_.halted = false;
        set_flag(CPU6809State::CC_I, true);
        set_flag(CPU6809State::CC_F, true);
        state_.pc = read16(0xFFF6);
        return cycles;
    }

    // IRQ — level-triggered, if I flag clear
    if (state_.irq_line && !get_flag(CPU6809State::CC_I)) {
        if (!state_.cwai_waiting) {
            push_entire_state();
            cycles += 19;
        } else {
            state_.cwai_waiting = false;
        }
        state_.halted = false;
        set_flag(CPU6809State::CC_I, true);
        state_.pc = read16(0xFFF8);
        return cycles;
    }

    return 0;
}

uint8_t CPU6809::execute_instruction() {
    // If halted (SYNC), check if interrupt wakes us
    if (state_.halted) {
        if (state_.nmi_pending ||
            (state_.firq_line && !get_flag(CPU6809State::CC_F)) ||
            (state_.irq_line && !get_flag(CPU6809State::CC_I))) {
            state_.halted = false;
            // Fall through to check_interrupts below
        } else {
            state_.clock_cycles += 1;
            return 1;  // Burn 1 cycle waiting
        }
    }

    // If CWAI waiting, don't execute instructions, just check interrupts
    if (state_.cwai_waiting) {
        uint8_t int_cycles = check_interrupts();
        if (int_cycles > 0) {
            state_.clock_cycles += int_cycles;
            return int_cycles;
        }
        state_.clock_cycles += 1;
        return 1;  // Burn 1 cycle waiting
    }

    uint8_t opcode = fetch();
    uint8_t cycles = 0;

    switch (opcode) {
        case 0x10:
            opcode = fetch();
            cycles = execute_page2(opcode);
            break;
        case 0x11:
            opcode = fetch();
            cycles = execute_page3(opcode);
            break;
        default:
            cycles = execute_page1(opcode);
            break;
    }

    state_.clock_cycles += cycles;

    // Check for pending interrupts after instruction execution
    uint8_t int_cycles = check_interrupts();
    cycles += int_cycles;
    state_.clock_cycles += int_cycles;

    return cycles;
}

uint8_t CPU6809::execute_page1(uint8_t opcode) {
    switch (opcode) {
        // --- 0x00-0x0F: Direct addressing memory ops ---
        case 0x00: { uint16_t ea = addr_direct(); write(ea, op_neg8(read(ea))); return 6; }
        case 0x01: return 2;  // illegal
        case 0x02: return 2;  // illegal
        case 0x03: { uint16_t ea = addr_direct(); write(ea, op_com8(read(ea))); return 6; }
        case 0x04: { uint16_t ea = addr_direct(); write(ea, op_lsr8(read(ea))); return 6; }
        case 0x05: return 2;  // illegal
        case 0x06: { uint16_t ea = addr_direct(); write(ea, op_ror8(read(ea))); return 6; }
        case 0x07: { uint16_t ea = addr_direct(); write(ea, op_asr8(read(ea))); return 6; }
        case 0x08: { uint16_t ea = addr_direct(); write(ea, op_asl8(read(ea))); return 6; }
        case 0x09: { uint16_t ea = addr_direct(); write(ea, op_rol8(read(ea))); return 6; }
        case 0x0A: { uint16_t ea = addr_direct(); write(ea, op_dec8(read(ea))); return 6; }
        case 0x0B: return 2;  // illegal
        case 0x0C: { uint16_t ea = addr_direct(); write(ea, op_inc8(read(ea))); return 6; }
        case 0x0D: { uint16_t ea = addr_direct(); op_tst8(read(ea)); return 6; }
        case 0x0E: { state_.pc = addr_direct(); return 3; }  // JMP direct
        case 0x0F: { uint16_t ea = addr_direct(); write(ea, op_clr8()); return 6; }

        // --- 0x10-0x1F: Misc ---
        case 0x12: return 2;  // NOP
        case 0x13: { // SYNC — halt until any interrupt
            state_.halted = true;
            return 4;
        }
        case 0x14: return 2;  // illegal
        case 0x15: return 2;  // illegal
        case 0x16: { int16_t off = static_cast<int16_t>(fetch16()); state_.pc += off; return 5; }  // LBRA
        case 0x17: { // LBSR
            int16_t off = static_cast<int16_t>(fetch16());
            write(--state_.s, state_.pc & 0xFF);
            write(--state_.s, state_.pc >> 8);
            state_.pc += off;
            return 9;
        }
        case 0x18: return 2;  // illegal
        case 0x19: { // DAA
            uint8_t correction = 0;
            bool new_carry = get_flag(CPU6809State::CC_C);
            if (get_flag(CPU6809State::CC_H) || (state_.a & 0x0F) > 9) correction |= 0x06;
            if (get_flag(CPU6809State::CC_C) || (state_.a & 0xF0) > 0x90 ||
                ((state_.a & 0xF0) > 0x80 && (state_.a & 0x0F) > 9)) {
                correction |= 0x60;
                new_carry = true;
            }
            uint16_t sum = static_cast<uint16_t>(state_.a) + correction;
            state_.a = static_cast<uint8_t>(sum);
            set_nz8(state_.a);
            set_flag(CPU6809State::CC_C, new_carry);
            // V is undefined after DAA
            return 2;
        }
        case 0x1A: { state_.cc |= fetch(); return 3; }  // ORCC immediate
        case 0x1B: return 2;  // illegal
        case 0x1C: { state_.cc &= fetch(); return 3; }  // ANDCC immediate
        case 0x1D: { // SEX
            state_.a = (state_.b & 0x80) ? 0xFF : 0x00;
            set_nz16(state_.d());
            return 2;
        }
        case 0x1E: { // EXG
            uint8_t postbyte = fetch();
            uint8_t r1 = (postbyte >> 4) & 0x0F;
            uint8_t r2 = postbyte & 0x0F;
            uint16_t tmp = get_register(r1);
            set_register(r1, get_register(r2));
            set_register(r2, tmp);
            return 8;
        }
        case 0x1F: { // TFR
            uint8_t postbyte = fetch();
            uint8_t src = (postbyte >> 4) & 0x0F;
            uint8_t dst = postbyte & 0x0F;
            set_register(dst, get_register(src));
            return 6;
        }

        // --- 0x20-0x2F: Short branches (stub — task 3.6) ---
        case 0x20: { int8_t off = static_cast<int8_t>(fetch()); state_.pc += off; return 3; }  // BRA
        case 0x21: { fetch(); return 3; }  // BRN
        case 0x22: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_C) && !get_flag(CPU6809State::CC_Z)) state_.pc += off; return 3; }  // BHI
        case 0x23: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_C) || get_flag(CPU6809State::CC_Z)) state_.pc += off; return 3; }  // BLS
        case 0x24: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_C)) state_.pc += off; return 3; }  // BCC
        case 0x25: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_C)) state_.pc += off; return 3; }  // BCS
        case 0x26: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_Z)) state_.pc += off; return 3; }  // BNE
        case 0x27: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_Z)) state_.pc += off; return 3; }  // BEQ
        case 0x28: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_V)) state_.pc += off; return 3; }  // BVC
        case 0x29: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_V)) state_.pc += off; return 3; }  // BVS
        case 0x2A: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_N)) state_.pc += off; return 3; }  // BPL
        case 0x2B: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_N)) state_.pc += off; return 3; }  // BMI
        case 0x2C: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_N) == get_flag(CPU6809State::CC_V)) state_.pc += off; return 3; }  // BGE
        case 0x2D: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_N) != get_flag(CPU6809State::CC_V)) state_.pc += off; return 3; }  // BLT
        case 0x2E: { int8_t off = static_cast<int8_t>(fetch()); if (!get_flag(CPU6809State::CC_Z) && (get_flag(CPU6809State::CC_N) == get_flag(CPU6809State::CC_V))) state_.pc += off; return 3; }  // BGT
        case 0x2F: { int8_t off = static_cast<int8_t>(fetch()); if (get_flag(CPU6809State::CC_Z) || (get_flag(CPU6809State::CC_N) != get_flag(CPU6809State::CC_V))) state_.pc += off; return 3; }  // BLE

        // --- 0x30-0x3F: Misc (stubs for task 3.5/3.7/3.8) ---
        case 0x30: { uint8_t ec; state_.x = addr_indexed(ec); set_flag(CPU6809State::CC_Z, state_.x == 0); return 4 + ec; }  // LEAX
        case 0x31: { uint8_t ec; state_.y = addr_indexed(ec); set_flag(CPU6809State::CC_Z, state_.y == 0); return 4 + ec; }  // LEAY
        case 0x32: { uint8_t ec; state_.s = addr_indexed(ec); return 4 + ec; }  // LEAS
        case 0x33: { uint8_t ec; state_.u = addr_indexed(ec); return 4 + ec; }  // LEAU
        case 0x34: { // PSHS
            uint8_t postbyte = fetch();
            uint8_t cycles = 5;
            if (postbyte & 0x80) { write(--state_.s, state_.pc & 0xFF); write(--state_.s, state_.pc >> 8); cycles += 2; }
            if (postbyte & 0x40) { write(--state_.s, state_.u & 0xFF); write(--state_.s, state_.u >> 8); cycles += 2; }
            if (postbyte & 0x20) { write(--state_.s, state_.y & 0xFF); write(--state_.s, state_.y >> 8); cycles += 2; }
            if (postbyte & 0x10) { write(--state_.s, state_.x & 0xFF); write(--state_.s, state_.x >> 8); cycles += 2; }
            if (postbyte & 0x08) { write(--state_.s, state_.dp); cycles += 1; }
            if (postbyte & 0x04) { write(--state_.s, state_.b); cycles += 1; }
            if (postbyte & 0x02) { write(--state_.s, state_.a); cycles += 1; }
            if (postbyte & 0x01) { write(--state_.s, state_.cc); cycles += 1; }
            return cycles;
        }
        case 0x35: { // PULS
            uint8_t postbyte = fetch();
            uint8_t cycles = 5;
            if (postbyte & 0x01) { state_.cc = read(state_.s++); cycles += 1; }
            if (postbyte & 0x02) { state_.a = read(state_.s++); cycles += 1; }
            if (postbyte & 0x04) { state_.b = read(state_.s++); cycles += 1; }
            if (postbyte & 0x08) { state_.dp = read(state_.s++); cycles += 1; }
            if (postbyte & 0x10) { uint8_t hi = read(state_.s++); uint8_t lo = read(state_.s++); state_.x = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x20) { uint8_t hi = read(state_.s++); uint8_t lo = read(state_.s++); state_.y = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x40) { uint8_t hi = read(state_.s++); uint8_t lo = read(state_.s++); state_.u = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x80) { uint8_t hi = read(state_.s++); uint8_t lo = read(state_.s++); state_.pc = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            return cycles;
        }
        case 0x36: { // PSHU
            uint8_t postbyte = fetch();
            uint8_t cycles = 5;
            if (postbyte & 0x80) { write(--state_.u, state_.pc & 0xFF); write(--state_.u, state_.pc >> 8); cycles += 2; }
            if (postbyte & 0x40) { write(--state_.u, state_.s & 0xFF); write(--state_.u, state_.s >> 8); cycles += 2; }
            if (postbyte & 0x20) { write(--state_.u, state_.y & 0xFF); write(--state_.u, state_.y >> 8); cycles += 2; }
            if (postbyte & 0x10) { write(--state_.u, state_.x & 0xFF); write(--state_.u, state_.x >> 8); cycles += 2; }
            if (postbyte & 0x08) { write(--state_.u, state_.dp); cycles += 1; }
            if (postbyte & 0x04) { write(--state_.u, state_.b); cycles += 1; }
            if (postbyte & 0x02) { write(--state_.u, state_.a); cycles += 1; }
            if (postbyte & 0x01) { write(--state_.u, state_.cc); cycles += 1; }
            return cycles;
        }
        case 0x37: { // PULU
            uint8_t postbyte = fetch();
            uint8_t cycles = 5;
            if (postbyte & 0x01) { state_.cc = read(state_.u++); cycles += 1; }
            if (postbyte & 0x02) { state_.a = read(state_.u++); cycles += 1; }
            if (postbyte & 0x04) { state_.b = read(state_.u++); cycles += 1; }
            if (postbyte & 0x08) { state_.dp = read(state_.u++); cycles += 1; }
            if (postbyte & 0x10) { uint8_t hi = read(state_.u++); uint8_t lo = read(state_.u++); state_.x = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x20) { uint8_t hi = read(state_.u++); uint8_t lo = read(state_.u++); state_.y = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x40) { uint8_t hi = read(state_.u++); uint8_t lo = read(state_.u++); state_.s = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            if (postbyte & 0x80) { uint8_t hi = read(state_.u++); uint8_t lo = read(state_.u++); state_.pc = (static_cast<uint16_t>(hi) << 8) | lo; cycles += 2; }
            return cycles;
        }
        case 0x38: return 2;  // illegal
        case 0x39: { // RTS
            uint8_t hi = read(state_.s++);
            uint8_t lo = read(state_.s++);
            state_.pc = (static_cast<uint16_t>(hi) << 8) | lo;
            return 5;
        }
        case 0x3A: { state_.x = state_.x + static_cast<uint16_t>(state_.b); return 3; }  // ABX
        case 0x3B: { // RTI
            state_.cc = read(state_.s++);
            if (get_flag(CPU6809State::CC_E)) {
                // Entire state was saved
                state_.a = read(state_.s++);
                state_.b = read(state_.s++);
                state_.dp = read(state_.s++);
                uint8_t xhi = read(state_.s++); uint8_t xlo = read(state_.s++);
                state_.x = (static_cast<uint16_t>(xhi) << 8) | xlo;
                uint8_t yhi = read(state_.s++); uint8_t ylo = read(state_.s++);
                state_.y = (static_cast<uint16_t>(yhi) << 8) | ylo;
                uint8_t uhi = read(state_.s++); uint8_t ulo = read(state_.s++);
                state_.u = (static_cast<uint16_t>(uhi) << 8) | ulo;
                uint8_t phi = read(state_.s++); uint8_t plo = read(state_.s++);
                state_.pc = (static_cast<uint16_t>(phi) << 8) | plo;
                return 15;
            } else {
                // Only CC and PC were saved (FIRQ)
                uint8_t phi = read(state_.s++); uint8_t plo = read(state_.s++);
                state_.pc = (static_cast<uint16_t>(phi) << 8) | plo;
                return 6;
            }
        }
        case 0x3C: { // CWAI — AND immediate with CC, push state, wait for interrupt
            uint8_t imm = fetch();
            state_.cc &= imm;
            push_entire_state();
            state_.cwai_waiting = true;
            return 20;
        }
        case 0x3D: { // MUL
            uint16_t result = static_cast<uint16_t>(state_.a) * static_cast<uint16_t>(state_.b);
            state_.set_d(result);
            set_flag(CPU6809State::CC_Z, result == 0);
            set_flag(CPU6809State::CC_C, (state_.b & 0x80) != 0);
            return 11;
        }
        case 0x3E: return 2;  // illegal
        case 0x3F: { // SWI
            push_entire_state();
            set_flag(CPU6809State::CC_I, true);
            set_flag(CPU6809State::CC_F, true);
            state_.pc = read16(0xFFFA);
            return 19;
        }

        // --- 0x40-0x4F: Inherent A register ops ---
        case 0x40: { state_.a = op_neg8(state_.a); return 2; }
        case 0x41: return 2;  // illegal
        case 0x42: return 2;  // illegal
        case 0x43: { state_.a = op_com8(state_.a); return 2; }
        case 0x44: { state_.a = op_lsr8(state_.a); return 2; }
        case 0x45: return 2;  // illegal
        case 0x46: { state_.a = op_ror8(state_.a); return 2; }
        case 0x47: { state_.a = op_asr8(state_.a); return 2; }
        case 0x48: { state_.a = op_asl8(state_.a); return 2; }
        case 0x49: { state_.a = op_rol8(state_.a); return 2; }
        case 0x4A: { state_.a = op_dec8(state_.a); return 2; }
        case 0x4B: return 2;  // illegal
        case 0x4C: { state_.a = op_inc8(state_.a); return 2; }
        case 0x4D: { op_tst8(state_.a); return 2; }
        case 0x4E: return 2;  // illegal
        case 0x4F: { state_.a = op_clr8(); return 2; }

        // --- 0x50-0x5F: Inherent B register ops ---
        case 0x50: { state_.b = op_neg8(state_.b); return 2; }
        case 0x51: return 2;  // illegal
        case 0x52: return 2;  // illegal
        case 0x53: { state_.b = op_com8(state_.b); return 2; }
        case 0x54: { state_.b = op_lsr8(state_.b); return 2; }
        case 0x55: return 2;  // illegal
        case 0x56: { state_.b = op_ror8(state_.b); return 2; }
        case 0x57: { state_.b = op_asr8(state_.b); return 2; }
        case 0x58: { state_.b = op_asl8(state_.b); return 2; }
        case 0x59: { state_.b = op_rol8(state_.b); return 2; }
        case 0x5A: { state_.b = op_dec8(state_.b); return 2; }
        case 0x5B: return 2;  // illegal
        case 0x5C: { state_.b = op_inc8(state_.b); return 2; }
        case 0x5D: { op_tst8(state_.b); return 2; }
        case 0x5E: return 2;  // illegal
        case 0x5F: { state_.b = op_clr8(); return 2; }

        // --- 0x60-0x6F: Indexed addressing memory ops ---
        case 0x60: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_neg8(read(ea))); return 6 + ec; }
        case 0x61: return 2;  // illegal
        case 0x62: return 2;  // illegal
        case 0x63: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_com8(read(ea))); return 6 + ec; }
        case 0x64: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_lsr8(read(ea))); return 6 + ec; }
        case 0x65: return 2;  // illegal
        case 0x66: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_ror8(read(ea))); return 6 + ec; }
        case 0x67: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_asr8(read(ea))); return 6 + ec; }
        case 0x68: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_asl8(read(ea))); return 6 + ec; }
        case 0x69: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_rol8(read(ea))); return 6 + ec; }
        case 0x6A: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_dec8(read(ea))); return 6 + ec; }
        case 0x6B: return 2;  // illegal
        case 0x6C: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_inc8(read(ea))); return 6 + ec; }
        case 0x6D: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_tst8(read(ea)); return 6 + ec; }
        case 0x6E: { uint8_t ec; state_.pc = addr_indexed(ec); return 3 + ec; }  // JMP indexed
        case 0x6F: { uint8_t ec; uint16_t ea = addr_indexed(ec); write(ea, op_clr8()); return 6 + ec; }

        // --- 0x70-0x7F: Extended addressing memory ops ---
        case 0x70: { uint16_t ea = addr_extended(); write(ea, op_neg8(read(ea))); return 7; }
        case 0x71: return 2;  // illegal
        case 0x72: return 2;  // illegal
        case 0x73: { uint16_t ea = addr_extended(); write(ea, op_com8(read(ea))); return 7; }
        case 0x74: { uint16_t ea = addr_extended(); write(ea, op_lsr8(read(ea))); return 7; }
        case 0x75: return 2;  // illegal
        case 0x76: { uint16_t ea = addr_extended(); write(ea, op_ror8(read(ea))); return 7; }
        case 0x77: { uint16_t ea = addr_extended(); write(ea, op_asr8(read(ea))); return 7; }
        case 0x78: { uint16_t ea = addr_extended(); write(ea, op_asl8(read(ea))); return 7; }
        case 0x79: { uint16_t ea = addr_extended(); write(ea, op_rol8(read(ea))); return 7; }
        case 0x7A: { uint16_t ea = addr_extended(); write(ea, op_dec8(read(ea))); return 7; }
        case 0x7B: return 2;  // illegal
        case 0x7C: { uint16_t ea = addr_extended(); write(ea, op_inc8(read(ea))); return 7; }
        case 0x7D: { uint16_t ea = addr_extended(); op_tst8(read(ea)); return 7; }
        case 0x7E: { state_.pc = addr_extended(); return 4; }  // JMP extended
        case 0x7F: { uint16_t ea = addr_extended(); write(ea, op_clr8()); return 7; }

        // --- 0x80-0x8F: Immediate A ops ---
        case 0x80: { state_.a = op_sub8(state_.a, fetch()); return 2; }           // SUBA imm
        case 0x81: { op_cmp8(state_.a, fetch()); return 2; }                      // CMPA imm
        case 0x82: { state_.a = op_sub8(state_.a, fetch(), get_flag(CPU6809State::CC_C)); return 2; }  // SBCA imm
        case 0x83: { uint16_t imm = fetch16(); state_.set_d(op_sub16(state_.d(), imm)); return 4; }  // SUBD imm
        case 0x84: { state_.a = op_and8(state_.a, fetch()); return 2; }           // ANDA imm
        case 0x85: { op_and8(state_.a, fetch()); return 2; }                      // BITA imm (AND, discard result)
        case 0x86: { op_ld8(state_.a, fetch()); return 2; }                       // LDA imm
        case 0x87: return 2;  // illegal
        case 0x88: { state_.a = op_eor8(state_.a, fetch()); return 2; }           // EORA imm
        case 0x89: { state_.a = op_add8(state_.a, fetch(), get_flag(CPU6809State::CC_C)); return 2; }  // ADCA imm
        case 0x8A: { state_.a = op_or8(state_.a, fetch()); return 2; }            // ORA imm
        case 0x8B: { state_.a = op_add8(state_.a, fetch()); return 2; }           // ADDA imm
        case 0x8C: { op_cmp16(state_.x, fetch16()); return 4; }  // CMPX imm
        case 0x8D: { // BSR
            int8_t off = static_cast<int8_t>(fetch());
            write(--state_.s, state_.pc & 0xFF);
            write(--state_.s, state_.pc >> 8);
            state_.pc += off;
            return 7;
        }
        case 0x8E: { op_ld16(state_.x, fetch16()); return 3; }  // LDX imm
        case 0x8F: return 2;  // illegal

        // --- 0x90-0x9F: Direct A ops ---
        case 0x90: { uint16_t ea = addr_direct(); state_.a = op_sub8(state_.a, read(ea)); return 4; }
        case 0x91: { uint16_t ea = addr_direct(); op_cmp8(state_.a, read(ea)); return 4; }
        case 0x92: { uint16_t ea = addr_direct(); state_.a = op_sub8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 4; }
        case 0x93: { uint16_t ea = addr_direct(); state_.set_d(op_sub16(state_.d(), read16(ea))); return 6; }  // SUBD direct
        case 0x94: { uint16_t ea = addr_direct(); state_.a = op_and8(state_.a, read(ea)); return 4; }
        case 0x95: { uint16_t ea = addr_direct(); op_and8(state_.a, read(ea)); return 4; }  // BITA
        case 0x96: { uint16_t ea = addr_direct(); op_ld8(state_.a, read(ea)); return 4; }
        case 0x97: { uint16_t ea = addr_direct(); op_st8(ea, state_.a); return 4; }
        case 0x98: { uint16_t ea = addr_direct(); state_.a = op_eor8(state_.a, read(ea)); return 4; }
        case 0x99: { uint16_t ea = addr_direct(); state_.a = op_add8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 4; }
        case 0x9A: { uint16_t ea = addr_direct(); state_.a = op_or8(state_.a, read(ea)); return 4; }
        case 0x9B: { uint16_t ea = addr_direct(); state_.a = op_add8(state_.a, read(ea)); return 4; }
        case 0x9C: { uint16_t ea = addr_direct(); op_cmp16(state_.x, read16(ea)); return 6; }  // CMPX direct
        case 0x9D: { // JSR direct (stub — task 3.7)
            uint16_t ea = addr_direct();
            write(--state_.s, state_.pc & 0xFF);
            write(--state_.s, state_.pc >> 8);
            state_.pc = ea;
            return 7;
        }
        case 0x9E: { uint16_t ea = addr_direct(); op_ld16(state_.x, read16(ea)); return 5; }  // LDX direct
        case 0x9F: { uint16_t ea = addr_direct(); op_st16(ea, state_.x); return 5; }  // STX direct

        // --- 0xA0-0xAF: Indexed A ops ---
        case 0xA0: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_sub8(state_.a, read(ea)); return 4 + ec; }
        case 0xA1: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp8(state_.a, read(ea)); return 4 + ec; }
        case 0xA2: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_sub8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 4 + ec; }
        case 0xA3: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.set_d(op_sub16(state_.d(), read16(ea))); return 6 + ec; }  // SUBD indexed
        case 0xA4: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_and8(state_.a, read(ea)); return 4 + ec; }
        case 0xA5: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_and8(state_.a, read(ea)); return 4 + ec; }  // BITA
        case 0xA6: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld8(state_.a, read(ea)); return 4 + ec; }
        case 0xA7: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st8(ea, state_.a); return 4 + ec; }
        case 0xA8: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_eor8(state_.a, read(ea)); return 4 + ec; }
        case 0xA9: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_add8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 4 + ec; }
        case 0xAA: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_or8(state_.a, read(ea)); return 4 + ec; }
        case 0xAB: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.a = op_add8(state_.a, read(ea)); return 4 + ec; }
        case 0xAC: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp16(state_.x, read16(ea)); return 6 + ec; }  // CMPX indexed
        case 0xAD: { // JSR indexed (stub — task 3.7)
            uint8_t ec; uint16_t ea = addr_indexed(ec);
            write(--state_.s, state_.pc & 0xFF);
            write(--state_.s, state_.pc >> 8);
            state_.pc = ea;
            return 7 + ec;
        }
        case 0xAE: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld16(state_.x, read16(ea)); return 5 + ec; }  // LDX indexed
        case 0xAF: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st16(ea, state_.x); return 5 + ec; }  // STX indexed

        // --- 0xB0-0xBF: Extended A ops ---
        case 0xB0: { uint16_t ea = addr_extended(); state_.a = op_sub8(state_.a, read(ea)); return 5; }
        case 0xB1: { uint16_t ea = addr_extended(); op_cmp8(state_.a, read(ea)); return 5; }
        case 0xB2: { uint16_t ea = addr_extended(); state_.a = op_sub8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 5; }
        case 0xB3: { uint16_t ea = addr_extended(); state_.set_d(op_sub16(state_.d(), read16(ea))); return 7; }  // SUBD extended
        case 0xB4: { uint16_t ea = addr_extended(); state_.a = op_and8(state_.a, read(ea)); return 5; }
        case 0xB5: { uint16_t ea = addr_extended(); op_and8(state_.a, read(ea)); return 5; }  // BITA
        case 0xB6: { uint16_t ea = addr_extended(); op_ld8(state_.a, read(ea)); return 5; }
        case 0xB7: { uint16_t ea = addr_extended(); op_st8(ea, state_.a); return 5; }
        case 0xB8: { uint16_t ea = addr_extended(); state_.a = op_eor8(state_.a, read(ea)); return 5; }
        case 0xB9: { uint16_t ea = addr_extended(); state_.a = op_add8(state_.a, read(ea), get_flag(CPU6809State::CC_C)); return 5; }
        case 0xBA: { uint16_t ea = addr_extended(); state_.a = op_or8(state_.a, read(ea)); return 5; }
        case 0xBB: { uint16_t ea = addr_extended(); state_.a = op_add8(state_.a, read(ea)); return 5; }
        case 0xBC: { uint16_t ea = addr_extended(); op_cmp16(state_.x, read16(ea)); return 7; }  // CMPX extended
        case 0xBD: { // JSR extended (stub — task 3.7)
            uint16_t ea = addr_extended();
            write(--state_.s, state_.pc & 0xFF);
            write(--state_.s, state_.pc >> 8);
            state_.pc = ea;
            return 8;
        }
        case 0xBE: { uint16_t ea = addr_extended(); op_ld16(state_.x, read16(ea)); return 6; }  // LDX extended
        case 0xBF: { uint16_t ea = addr_extended(); op_st16(ea, state_.x); return 6; }  // STX extended

        // --- 0xC0-0xCF: Immediate B ops ---
        case 0xC0: { state_.b = op_sub8(state_.b, fetch()); return 2; }           // SUBB imm
        case 0xC1: { op_cmp8(state_.b, fetch()); return 2; }                      // CMPB imm
        case 0xC2: { state_.b = op_sub8(state_.b, fetch(), get_flag(CPU6809State::CC_C)); return 2; }  // SBCB imm
        case 0xC3: { uint16_t imm = fetch16(); state_.set_d(op_add16(state_.d(), imm)); return 4; }  // ADDD imm
        case 0xC4: { state_.b = op_and8(state_.b, fetch()); return 2; }           // ANDB imm
        case 0xC5: { op_and8(state_.b, fetch()); return 2; }                      // BITB imm
        case 0xC6: { op_ld8(state_.b, fetch()); return 2; }                       // LDB imm
        case 0xC7: return 2;  // illegal
        case 0xC8: { state_.b = op_eor8(state_.b, fetch()); return 2; }           // EORB imm
        case 0xC9: { state_.b = op_add8(state_.b, fetch(), get_flag(CPU6809State::CC_C)); return 2; }  // ADCB imm
        case 0xCA: { state_.b = op_or8(state_.b, fetch()); return 2; }            // ORB imm
        case 0xCB: { state_.b = op_add8(state_.b, fetch()); return 2; }           // ADDB imm
        case 0xCC: { uint16_t tmp = state_.d(); op_ld16(tmp, fetch16()); state_.set_d(tmp); return 3; }  // LDD imm
        case 0xCD: return 2;  // illegal
        case 0xCE: { op_ld16(state_.u, fetch16()); return 3; }  // LDU imm
        case 0xCF: return 2;  // illegal

        // --- 0xD0-0xDF: Direct B ops ---
        case 0xD0: { uint16_t ea = addr_direct(); state_.b = op_sub8(state_.b, read(ea)); return 4; }
        case 0xD1: { uint16_t ea = addr_direct(); op_cmp8(state_.b, read(ea)); return 4; }
        case 0xD2: { uint16_t ea = addr_direct(); state_.b = op_sub8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 4; }
        case 0xD3: { uint16_t ea = addr_direct(); state_.set_d(op_add16(state_.d(), read16(ea))); return 6; }  // ADDD direct
        case 0xD4: { uint16_t ea = addr_direct(); state_.b = op_and8(state_.b, read(ea)); return 4; }
        case 0xD5: { uint16_t ea = addr_direct(); op_and8(state_.b, read(ea)); return 4; }  // BITB
        case 0xD6: { uint16_t ea = addr_direct(); op_ld8(state_.b, read(ea)); return 4; }
        case 0xD7: { uint16_t ea = addr_direct(); op_st8(ea, state_.b); return 4; }
        case 0xD8: { uint16_t ea = addr_direct(); state_.b = op_eor8(state_.b, read(ea)); return 4; }
        case 0xD9: { uint16_t ea = addr_direct(); state_.b = op_add8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 4; }
        case 0xDA: { uint16_t ea = addr_direct(); state_.b = op_or8(state_.b, read(ea)); return 4; }
        case 0xDB: { uint16_t ea = addr_direct(); state_.b = op_add8(state_.b, read(ea)); return 4; }
        case 0xDC: { uint16_t ea = addr_direct(); uint16_t tmp = state_.d(); op_ld16(tmp, read16(ea)); state_.set_d(tmp); return 5; }  // LDD direct
        case 0xDD: { uint16_t ea = addr_direct(); op_st16(ea, state_.d()); return 5; }  // STD direct
        case 0xDE: { uint16_t ea = addr_direct(); op_ld16(state_.u, read16(ea)); return 5; }  // LDU direct
        case 0xDF: { uint16_t ea = addr_direct(); op_st16(ea, state_.u); return 5; }  // STU direct

        // --- 0xE0-0xEF: Indexed B ops ---
        case 0xE0: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_sub8(state_.b, read(ea)); return 4 + ec; }
        case 0xE1: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp8(state_.b, read(ea)); return 4 + ec; }
        case 0xE2: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_sub8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 4 + ec; }
        case 0xE3: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.set_d(op_add16(state_.d(), read16(ea))); return 6 + ec; }  // ADDD indexed
        case 0xE4: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_and8(state_.b, read(ea)); return 4 + ec; }
        case 0xE5: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_and8(state_.b, read(ea)); return 4 + ec; }  // BITB
        case 0xE6: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld8(state_.b, read(ea)); return 4 + ec; }
        case 0xE7: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st8(ea, state_.b); return 4 + ec; }
        case 0xE8: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_eor8(state_.b, read(ea)); return 4 + ec; }
        case 0xE9: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_add8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 4 + ec; }
        case 0xEA: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_or8(state_.b, read(ea)); return 4 + ec; }
        case 0xEB: { uint8_t ec; uint16_t ea = addr_indexed(ec); state_.b = op_add8(state_.b, read(ea)); return 4 + ec; }
        case 0xEC: { uint8_t ec; uint16_t ea = addr_indexed(ec); uint16_t tmp = state_.d(); op_ld16(tmp, read16(ea)); state_.set_d(tmp); return 5 + ec; }  // LDD indexed
        case 0xED: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st16(ea, state_.d()); return 5 + ec; }  // STD indexed
        case 0xEE: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld16(state_.u, read16(ea)); return 5 + ec; }  // LDU indexed
        case 0xEF: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st16(ea, state_.u); return 5 + ec; }  // STU indexed

        // --- 0xF0-0xFF: Extended B ops ---
        case 0xF0: { uint16_t ea = addr_extended(); state_.b = op_sub8(state_.b, read(ea)); return 5; }
        case 0xF1: { uint16_t ea = addr_extended(); op_cmp8(state_.b, read(ea)); return 5; }
        case 0xF2: { uint16_t ea = addr_extended(); state_.b = op_sub8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 5; }
        case 0xF3: { uint16_t ea = addr_extended(); state_.set_d(op_add16(state_.d(), read16(ea))); return 7; }  // ADDD extended
        case 0xF4: { uint16_t ea = addr_extended(); state_.b = op_and8(state_.b, read(ea)); return 5; }
        case 0xF5: { uint16_t ea = addr_extended(); op_and8(state_.b, read(ea)); return 5; }  // BITB
        case 0xF6: { uint16_t ea = addr_extended(); op_ld8(state_.b, read(ea)); return 5; }
        case 0xF7: { uint16_t ea = addr_extended(); op_st8(ea, state_.b); return 5; }
        case 0xF8: { uint16_t ea = addr_extended(); state_.b = op_eor8(state_.b, read(ea)); return 5; }
        case 0xF9: { uint16_t ea = addr_extended(); state_.b = op_add8(state_.b, read(ea), get_flag(CPU6809State::CC_C)); return 5; }
        case 0xFA: { uint16_t ea = addr_extended(); state_.b = op_or8(state_.b, read(ea)); return 5; }
        case 0xFB: { uint16_t ea = addr_extended(); state_.b = op_add8(state_.b, read(ea)); return 5; }
        case 0xFC: { uint16_t ea = addr_extended(); uint16_t tmp = state_.d(); op_ld16(tmp, read16(ea)); state_.set_d(tmp); return 6; }  // LDD extended
        case 0xFD: { uint16_t ea = addr_extended(); op_st16(ea, state_.d()); return 6; }  // STD extended
        case 0xFE: { uint16_t ea = addr_extended(); op_ld16(state_.u, read16(ea)); return 6; }  // LDU extended
        case 0xFF: { uint16_t ea = addr_extended(); op_st16(ea, state_.u); return 6; }  // STU extended

        default: return 2;  // Illegal opcode
    }
}

uint8_t CPU6809::execute_page2(uint8_t opcode) {
    switch (opcode) {
        // Long branches
        case 0x21: { fetch16(); return 5; }  // LBRN
        case 0x22: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_C) && !get_flag(CPU6809State::CC_Z)) { state_.pc += off; return 6; } return 5; }  // LBHI
        case 0x23: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_C) || get_flag(CPU6809State::CC_Z)) { state_.pc += off; return 6; } return 5; }  // LBLS
        case 0x24: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_C)) { state_.pc += off; return 6; } return 5; }  // LBCC
        case 0x25: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_C)) { state_.pc += off; return 6; } return 5; }  // LBCS
        case 0x26: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_Z)) { state_.pc += off; return 6; } return 5; }  // LBNE
        case 0x27: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_Z)) { state_.pc += off; return 6; } return 5; }  // LBEQ
        case 0x28: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_V)) { state_.pc += off; return 6; } return 5; }  // LBVC
        case 0x29: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_V)) { state_.pc += off; return 6; } return 5; }  // LBVS
        case 0x2A: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_N)) { state_.pc += off; return 6; } return 5; }  // LBPL
        case 0x2B: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_N)) { state_.pc += off; return 6; } return 5; }  // LBMI
        case 0x2C: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_N) == get_flag(CPU6809State::CC_V)) { state_.pc += off; return 6; } return 5; }  // LBGE
        case 0x2D: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_N) != get_flag(CPU6809State::CC_V)) { state_.pc += off; return 6; } return 5; }  // LBLT
        case 0x2E: { int16_t off = static_cast<int16_t>(fetch16()); if (!get_flag(CPU6809State::CC_Z) && (get_flag(CPU6809State::CC_N) == get_flag(CPU6809State::CC_V))) { state_.pc += off; return 6; } return 5; }  // LBGT
        case 0x2F: { int16_t off = static_cast<int16_t>(fetch16()); if (get_flag(CPU6809State::CC_Z) || (get_flag(CPU6809State::CC_N) != get_flag(CPU6809State::CC_V))) { state_.pc += off; return 6; } return 5; }  // LBLE

        case 0x3F: { // SWI2
            push_entire_state();
            state_.pc = read16(0xFFF2);
            return 20;
        }

        // CMPD
        case 0x83: { op_cmp16(state_.d(), fetch16()); return 5; }
        case 0x93: { uint16_t ea = addr_direct(); op_cmp16(state_.d(), read16(ea)); return 7; }
        case 0xA3: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp16(state_.d(), read16(ea)); return 7 + ec; }
        case 0xB3: { uint16_t ea = addr_extended(); op_cmp16(state_.d(), read16(ea)); return 8; }

        // CMPY
        case 0x8C: { op_cmp16(state_.y, fetch16()); return 5; }
        case 0x9C: { uint16_t ea = addr_direct(); op_cmp16(state_.y, read16(ea)); return 7; }
        case 0xAC: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp16(state_.y, read16(ea)); return 7 + ec; }
        case 0xBC: { uint16_t ea = addr_extended(); op_cmp16(state_.y, read16(ea)); return 8; }

        // LDY
        case 0x8E: { op_ld16(state_.y, fetch16()); return 4; }
        case 0x9E: { uint16_t ea = addr_direct(); op_ld16(state_.y, read16(ea)); return 6; }
        case 0xAE: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld16(state_.y, read16(ea)); return 6 + ec; }
        case 0xBE: { uint16_t ea = addr_extended(); op_ld16(state_.y, read16(ea)); return 7; }

        // STY
        case 0x9F: { uint16_t ea = addr_direct(); op_st16(ea, state_.y); return 6; }
        case 0xAF: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st16(ea, state_.y); return 6 + ec; }
        case 0xBF: { uint16_t ea = addr_extended(); op_st16(ea, state_.y); return 7; }

        // LDS
        case 0xCE: { op_ld16(state_.s, fetch16()); return 4; }
        case 0xDE: { uint16_t ea = addr_direct(); op_ld16(state_.s, read16(ea)); return 6; }
        case 0xEE: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_ld16(state_.s, read16(ea)); return 6 + ec; }
        case 0xFE: { uint16_t ea = addr_extended(); op_ld16(state_.s, read16(ea)); return 7; }

        // STS
        case 0xDF: { uint16_t ea = addr_direct(); op_st16(ea, state_.s); return 6; }
        case 0xEF: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_st16(ea, state_.s); return 6 + ec; }
        case 0xFF: { uint16_t ea = addr_extended(); op_st16(ea, state_.s); return 7; }

        default: return 2;
    }
}

uint8_t CPU6809::execute_page3(uint8_t opcode) {
    switch (opcode) {
        case 0x3F: { // SWI3
            push_entire_state();
            state_.pc = read16(0xFFF4);
            return 20;
        }

        // CMPU
        case 0x83: { op_cmp16(state_.u, fetch16()); return 5; }
        case 0x93: { uint16_t ea = addr_direct(); op_cmp16(state_.u, read16(ea)); return 7; }
        case 0xA3: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp16(state_.u, read16(ea)); return 7 + ec; }
        case 0xB3: { uint16_t ea = addr_extended(); op_cmp16(state_.u, read16(ea)); return 8; }

        // CMPS
        case 0x8C: { op_cmp16(state_.s, fetch16()); return 5; }
        case 0x9C: { uint16_t ea = addr_direct(); op_cmp16(state_.s, read16(ea)); return 7; }
        case 0xAC: { uint8_t ec; uint16_t ea = addr_indexed(ec); op_cmp16(state_.s, read16(ea)); return 7 + ec; }
        case 0xBC: { uint16_t ea = addr_extended(); op_cmp16(state_.s, read16(ea)); return 8; }

        default: return 2;
    }
}

void CPU6809::set_memory_system(MemorySystem* mem) { memory_ = mem; }

uint8_t CPU6809::read(uint16_t address) {
    if (memory_) return memory_->read(address);
    return 0xFF;
}

void CPU6809::write(uint16_t address, uint8_t value) {
    if (memory_) memory_->write(address, value);
}

void CPU6809::assert_irq(bool active) { state_.irq_line = active; }
void CPU6809::assert_firq(bool active) { state_.firq_line = active; }
void CPU6809::assert_nmi() { state_.nmi_pending = true; }

CPU6809State CPU6809::get_state() const { return state_; }
void CPU6809::set_state(const CPU6809State& state) { state_ = state; }

} // namespace crayon
