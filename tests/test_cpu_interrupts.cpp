#include <gtest/gtest.h>
#include "cpu6809.h"
#include "memory_system.h"
#include <cstring>

using namespace crayon;

class InterruptTestFixture : public ::testing::Test {
protected:
    CPU6809 cpu;
    MemorySystem mem;
    uint8_t monitor_rom[0x1000];  // 4KB monitor ROM (0xF000-0xFFFF)

    void SetUp() override {
        cpu.set_memory_system(&mem);
        std::memset(monitor_rom, 0x00, sizeof(monitor_rom));
    }

    // Set a 16-bit vector in the monitor ROM image, then reload it
    void set_vector(uint16_t vector_addr, uint16_t target) {
        uint16_t offset = vector_addr - 0xF000;
        monitor_rom[offset] = static_cast<uint8_t>(target >> 8);
        monitor_rom[offset + 1] = static_cast<uint8_t>(target & 0xFF);
    }

    void load_rom() {
        mem.load_monitor_rom(monitor_rom, sizeof(monitor_rom));
    }

    // Set up CPU with known state at a given PC
    void setup_cpu(uint16_t pc, uint16_t sp = 0x8000) {
        auto state = cpu.get_state();
        state.pc = pc;
        state.s = sp;
        state.a = 0x11;
        state.b = 0x22;
        state.x = 0x3344;
        state.y = 0x5566;
        state.u = 0x7788;
        state.dp = 0x99;
        state.cc = 0x00;  // All flags clear (interrupts enabled)
        state.clock_cycles = 0;
        state.halted = false;
        state.cwai_waiting = false;
        state.nmi_pending = false;
        state.nmi_armed = false;
        state.irq_line = false;
        state.firq_line = false;
        cpu.set_state(state);
    }
};

// --- SWI Tests ---

TEST_F(InterruptTestFixture, SWI_PushesEntireStateAndVectors) {
    set_vector(0xFFFA, 0x2000);
    load_rom();
    setup_cpu(0x1000);
    // Place SWI opcode at 0x1000 (user RAM)
    mem.write(0x1000, 0x3F);

    uint8_t cycles = cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x2000);
    EXPECT_TRUE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_F) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
    // S moved down 12 bytes (CC,A,B,DP,X,Y,U,PC)
    EXPECT_EQ(state.s, 0x8000 - 12);
    // Verify pushed PC (0x1001 = after SWI opcode)
    EXPECT_EQ(mem.read(0x8000 - 2), 0x10);  // PC high
    EXPECT_EQ(mem.read(0x8000 - 1), 0x01);  // PC low
    EXPECT_GE(cycles, 19);
}

TEST_F(InterruptTestFixture, SWI2_PushesEntireStateAndVectors) {
    set_vector(0xFFF2, 0x3000);
    load_rom();
    setup_cpu(0x1000);
    // SWI2 = 0x10 0x3F
    mem.write(0x1000, 0x10);
    mem.write(0x1001, 0x3F);

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x3000);
    // SWI2 does NOT mask I or F
    EXPECT_FALSE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_FALSE((state.cc & CPU6809State::CC_F) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
    EXPECT_EQ(state.s, 0x8000 - 12);
}

TEST_F(InterruptTestFixture, SWI3_PushesEntireStateAndVectors) {
    set_vector(0xFFF4, 0x4000);
    load_rom();
    setup_cpu(0x1000);
    // SWI3 = 0x11 0x3F
    mem.write(0x1000, 0x11);
    mem.write(0x1001, 0x3F);

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x4000);
    EXPECT_FALSE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_FALSE((state.cc & CPU6809State::CC_F) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
    EXPECT_EQ(state.s, 0x8000 - 12);
}

// --- IRQ Tests ---

TEST_F(InterruptTestFixture, IRQ_WhenIFlagClear_PushesAndVectors) {
    set_vector(0xFFF8, 0x5000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_irq(true);

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x5000);
    EXPECT_TRUE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
}

TEST_F(InterruptTestFixture, IRQ_WhenIFlagSet_DoesNotVector) {
    set_vector(0xFFF8, 0x5000);
    load_rom();
    setup_cpu(0x1000);
    auto state = cpu.get_state();
    state.cc |= CPU6809State::CC_I;
    cpu.set_state(state);

    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_irq(true);

    cpu.execute_instruction();

    state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x1001);
}

// --- FIRQ Tests ---

TEST_F(InterruptTestFixture, FIRQ_WhenFFlagClear_PushesCCAndPCOnly) {
    set_vector(0xFFF6, 0x6000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_firq(true);

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x6000);
    EXPECT_TRUE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_F) != 0);
    // E flag should be CLEAR (only CC+PC pushed)
    EXPECT_FALSE((state.cc & CPU6809State::CC_E) != 0);
    // S moved down only 3 bytes (CC + PC hi + PC lo)
    EXPECT_EQ(state.s, 0x8000 - 3);
}

TEST_F(InterruptTestFixture, FIRQ_WhenFFlagSet_DoesNotVector) {
    set_vector(0xFFF6, 0x6000);
    load_rom();
    setup_cpu(0x1000);
    auto state = cpu.get_state();
    state.cc |= CPU6809State::CC_F;
    cpu.set_state(state);

    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_firq(true);

    cpu.execute_instruction();

    state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x1001);
}

// --- NMI Tests ---

TEST_F(InterruptTestFixture, NMI_WhenArmed_PushesEntireStateAndVectors) {
    set_vector(0xFFFC, 0x7000);
    load_rom();
    setup_cpu(0x1000);
    auto state = cpu.get_state();
    state.nmi_armed = true;
    cpu.set_state(state);

    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_nmi();

    cpu.execute_instruction();

    state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x7000);
    EXPECT_TRUE((state.cc & CPU6809State::CC_I) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_F) != 0);
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
    EXPECT_FALSE(state.nmi_pending);
}

TEST_F(InterruptTestFixture, NMI_WhenNotArmed_DoesNotVector) {
    set_vector(0xFFFC, 0x7000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_nmi();

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x1001);
}

// --- RTI Tests ---

TEST_F(InterruptTestFixture, RTI_WithEFlag_RestoresEntireState) {
    set_vector(0xFFFA, 0x2000);
    load_rom();
    setup_cpu(0x1000);
    // SWI pushes entire state
    mem.write(0x1000, 0x3F);  // SWI
    cpu.execute_instruction();

    // Place RTI at the SWI handler address
    mem.write(0x2000, 0x3B);  // RTI
    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x1001);
    EXPECT_EQ(state.a, 0x11);
    EXPECT_EQ(state.b, 0x22);
    EXPECT_EQ(state.x, 0x3344);
    EXPECT_EQ(state.y, 0x5566);
    EXPECT_EQ(state.u, 0x7788);
    EXPECT_EQ(state.dp, 0x99);
}

TEST_F(InterruptTestFixture, RTI_WithoutEFlag_RestoresOnlyCCAndPC) {
    set_vector(0xFFF6, 0x6000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_firq(true);
    cpu.execute_instruction();

    // Deassert FIRQ so it doesn't re-trigger
    cpu.assert_firq(false);

    // Place RTI at FIRQ handler
    mem.write(0x6000, 0x3B);  // RTI
    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x1001);
    EXPECT_EQ(state.s, 0x8000);
}

// --- SYNC Tests ---

TEST_F(InterruptTestFixture, SYNC_HaltsCPU) {
    setup_cpu(0x1000);
    mem.write(0x1000, 0x13);  // SYNC

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_TRUE(state.halted);
    EXPECT_EQ(state.pc, 0x1001);
}

TEST_F(InterruptTestFixture, SYNC_WakesOnIRQ) {
    set_vector(0xFFF8, 0x5000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x13);  // SYNC

    cpu.execute_instruction();
    EXPECT_TRUE(cpu.get_state().halted);

    // Assert IRQ — next execute should wake and vector
    cpu.assert_irq(true);
    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_FALSE(state.halted);
    EXPECT_EQ(state.pc, 0x5000);
}

// --- CWAI Tests ---

TEST_F(InterruptTestFixture, CWAI_ANDsImmediateAndWaits) {
    setup_cpu(0x1000);
    auto state = cpu.get_state();
    state.cc = 0xFF;  // All flags set
    cpu.set_state(state);

    mem.write(0x1000, 0x3C);  // CWAI
    mem.write(0x1001, 0x0F);  // AND mask: clears upper nibble

    cpu.execute_instruction();

    state = cpu.get_state();
    EXPECT_TRUE(state.cwai_waiting);
    // E flag set by push_entire_state
    EXPECT_TRUE((state.cc & CPU6809State::CC_E) != 0);
    EXPECT_EQ(state.s, 0x8000 - 12);
}

TEST_F(InterruptTestFixture, CWAI_WakesOnIRQ) {
    set_vector(0xFFF8, 0x5000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x3C);  // CWAI
    mem.write(0x1001, 0xEF);  // AND mask: clear I flag (bit 4)

    cpu.execute_instruction();
    EXPECT_TRUE(cpu.get_state().cwai_waiting);

    // Assert IRQ — should wake and vector (state already pushed)
    cpu.assert_irq(true);
    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_FALSE(state.cwai_waiting);
    EXPECT_EQ(state.pc, 0x5000);
}

// --- Priority Tests ---

TEST_F(InterruptTestFixture, NMI_HasHigherPriorityThanFIRQ) {
    set_vector(0xFFFC, 0x7000);
    set_vector(0xFFF6, 0x6000);
    load_rom();
    setup_cpu(0x1000);
    auto state = cpu.get_state();
    state.nmi_armed = true;
    cpu.set_state(state);

    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_nmi();
    cpu.assert_firq(true);

    cpu.execute_instruction();

    state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x7000);
}

TEST_F(InterruptTestFixture, FIRQ_HasHigherPriorityThanIRQ) {
    set_vector(0xFFF6, 0x6000);
    set_vector(0xFFF8, 0x5000);
    load_rom();
    setup_cpu(0x1000);
    mem.write(0x1000, 0x12);  // NOP
    cpu.assert_firq(true);
    cpu.assert_irq(true);

    cpu.execute_instruction();

    auto state = cpu.get_state();
    EXPECT_EQ(state.pc, 0x6000);
}
