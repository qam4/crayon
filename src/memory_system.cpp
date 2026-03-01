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
    if (size == 0 || size > sizeof(state_.basic_rom))
        return Result<void>::err("BASIC ROM wrong size (expected up to 12KB, got " + std::to_string(size) + " bytes)");
    std::memset(state_.basic_rom, 0xFF, sizeof(state_.basic_rom));
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
    if (size == 0 || size > sizeof(state_.monitor_rom))
        return Result<void>::err("Monitor ROM wrong size (expected up to 4KB, got " + std::to_string(size) + " bytes)");
    std::memset(state_.monitor_rom, 0xFF, sizeof(state_.monitor_rom));
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
    if (address < 0x4000) {
        // Video RAM: 0x0000-0x1FFF pixel data, 0x2000-0x3FFF color attributes
        return state_.video_ram[address];
    }
    if (address < 0xA000) {
        // 0x4000-0x5FFF: always user RAM
        // 0x6000-0x9FFF: cartridge ROM when inserted, else user RAM
        if (state_.cartridge_inserted && address >= 0x6000) {
            uint16_t cart_offset = address - 0x6000;
            if (cart_offset < state_.cartridge_rom.size())
                return state_.cartridge_rom[cart_offset];
            return 0xFF;
        }
        return state_.user_ram[address - 0x4000];
    }
    if (address < 0xA800) {
        // I/O space — PIA registers at 0xA7C0-0xA7C3
        if (pia_ && address >= 0xA7C0 && address <= 0xA7C3)
            return pia_->read_register(address & 0x03);
        return 0xFF;
    }
    if (address < 0xC000) {
        // Reserved: 0xA800-0xBFFF returns 0xFF
        return 0xFF;
    }
    if (address < 0xF000) {
        // BASIC ROM: 0xC000-0xEFFF
        return state_.basic_rom[address - 0xC000];
    }
    // Monitor ROM: 0xF000-0xFFFF
    return state_.monitor_rom[address - 0xF000];
}

void MemorySystem::write(uint16_t address, uint8_t value) {
    if (address < 0x4000) { state_.video_ram[address] = value; return; }
    if (address < 0xA000) {
        // Cartridge area: writes ignored when cartridge inserted
        if (state_.cartridge_inserted && address >= 0x6000) return;
        uint16_t offset = address - 0x4000;
        if (offset < sizeof(state_.user_ram)) state_.user_ram[offset] = value;
        return;
    }
    if (address < 0xA800) {
        // I/O space — PIA registers at 0xA7C0-0xA7C3
        if (pia_ && address >= 0xA7C0 && address <= 0xA7C3)
            pia_->write_register(address & 0x03, value);
        return;
    }
    // 0xA800-0xBFFF: reserved, writes ignored
    // 0xC000-0xFFFF: ROM, writes ignored
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
