#include "cpu6809.h"
#include "memory_system.h"

namespace crayon {

CPU6809::CPU6809() { reset(); }

void CPU6809::reset() {
    state_ = CPU6809State{};
    if (memory_) {
        // Read reset vector from 0xFFFE-0xFFFF
        uint8_t hi = read(0xFFFE);
        uint8_t lo = read(0xFFFF);
        state_.pc = (static_cast<uint16_t>(hi) << 8) | lo;
    }
    state_.cc = CPU6809State::CC_I | CPU6809State::CC_F;  // Interrupts masked on reset
}

uint8_t CPU6809::execute_instruction() {
    // TODO: Implement 6809 instruction decoding and execution
    // Stub: read opcode, consume 1 cycle, advance PC
    uint8_t opcode = read(state_.pc++);
    state_.clock_cycles++;
    (void)opcode;
    return 1;
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
