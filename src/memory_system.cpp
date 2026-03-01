#include "memory_system.h"
#include "pia.h"
#include "gate_array.h"
#include <cstring>
#include <fstream>
#include <iterator>

namespace crayon {

MemorySystem::MemorySystem() = default;

Result<void> MemorySystem::load_basic_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open BASIC ROM: " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), {});
    return load_basic_rom(data.data(), data.size());
}

Result<void> MemorySystem::load_basic_rom(const uint8_t* data, size_t size) {
    if (size > sizeof(state_.basic_rom))
        return Result<void>::err("BASIC ROM too large");
    std::memcpy(state_.basic_rom, data, size);
    state_.basic_rom_loaded = true;
    return Result<void>::ok();
}

Result<void> MemorySystem::load_monitor_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open Monitor ROM: " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), {});
    return load_monitor_rom(data.data(), data.size());
}

Result<void> MemorySystem::load_monitor_rom(const uint8_t* data, size_t size) {
    if (size > sizeof(state_.monitor_rom))
        return Result<void>::err("Monitor ROM too large");
    std::memcpy(state_.monitor_rom, data, size);
    state_.monitor_rom_loaded = true;
    return Result<void>::ok();
}

Result<void> MemorySystem::load_cartridge(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open cartridge: " + path);
    state_.cartridge_rom.assign((std::istreambuf_iterator<char>(file)), {});
    state_.cartridge_inserted = true;
    return Result<void>::ok();
}

Result<void> MemorySystem::load_cartridge(const uint8_t* data, size_t size) {
    state_.cartridge_rom.assign(data, data + size);
    state_.cartridge_inserted = true;
    return Result<void>::ok();
}

uint8_t MemorySystem::read(uint16_t address) {
    if (address < 0x2000) return state_.video_ram[address];
    if (address < 0x4000) return state_.video_ram[address];
    if (address < 0xA000) {
        uint16_t offset = address - 0x4000;
        if (state_.cartridge_inserted && address >= 0x6000 && address < 0xA000) {
            uint16_t cart_offset = address - 0x6000;
            if (cart_offset < state_.cartridge_rom.size())
                return state_.cartridge_rom[cart_offset];
        }
        if (offset < sizeof(state_.user_ram)) return state_.user_ram[offset];
        return 0xFF;
    }
    if (address >= 0xA000 && address < 0xA800) {
        // I/O space — PIA registers at 0xA7C0-0xA7C3
        if (pia_ && address >= 0xA7C0 && address <= 0xA7C3)
            return pia_->read_register(address & 0x03);
        return 0xFF;
    }
    if (address >= 0xC000 && address < 0xF000)
        return state_.basic_rom[address - 0xC000];
    if (address >= 0xF000)
        return state_.monitor_rom[address - 0xF000];
    return 0xFF;
}

void MemorySystem::write(uint16_t address, uint8_t value) {
    if (address < 0x4000) { state_.video_ram[address] = value; return; }
    if (address < 0xA000) {
        uint16_t offset = address - 0x4000;
        if (offset < sizeof(state_.user_ram)) state_.user_ram[offset] = value;
        return;
    }
    if (address >= 0xA000 && address < 0xA800) {
        if (pia_ && address >= 0xA7C0 && address <= 0xA7C3)
            pia_->write_register(address & 0x03, value);
        return;
    }
    // ROM writes ignored
}

const uint8_t* MemorySystem::get_pixel_ram() const { return state_.video_ram; }
const uint8_t* MemorySystem::get_color_ram() const { return state_.video_ram + 0x2000; }

void MemorySystem::set_pia(PIA* pia) { pia_ = pia; }
void MemorySystem::set_gate_array(GateArray* ga) { gate_array_ = ga; }

void MemorySystem::insert_cartridge() { state_.cartridge_inserted = true; }
void MemorySystem::remove_cartridge() { state_.cartridge_inserted = false; }
bool MemorySystem::has_cartridge() const { return state_.cartridge_inserted; }

MO5MemoryState MemorySystem::get_state() const { return state_; }
void MemorySystem::set_state(const MO5MemoryState& state) { state_ = state; }

} // namespace crayon
