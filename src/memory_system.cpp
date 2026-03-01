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
    if (address < 0x2000) {
        // Video RAM window: 0x0000-0x1FFF
        // Page selected by bit 0 of gate array register (0xA7C0)
        return state_.video_ram[address + (state_.video_page << 13)];
    }
    if (address < 0xA000) {
        // User RAM: 0x2000-0x9FFF (32KB)
        return state_.user_ram[address - 0x2000];
    }
    if (address < 0xA800) {
        // I/O space: 0xA000-0xA7FF
        if (address == 0xA7C0) {
            // Gate array system register — NOT a PIA register on MO5
            // Bit 7: always 1 (active-low cassette input)
            // Bit 6: not used
            // Bit 5: lightpen button (directly from hardware)
            // Bits 1-4: border color
            // Bit 0: video page select
            return state_.gate_array_reg | 0x80;
        }
        if (pia_ && address >= 0xA7C1 && address <= 0xA7C3) {
            // MO5 PIA register mapping (address lines swapped vs standard 6821):
            //   0xA7C1 → PIA reg 2 (Port B data/DDR — keyboard scan)
            //   0xA7C2 → PIA reg 1 (CRA)
            //   0xA7C3 → PIA reg 3 (CRB — vsync interrupt)
            static constexpr uint8_t mo5_pia_map[4] = {0, 2, 1, 3};
            return pia_->read_register(mo5_pia_map[address & 0x03]);
        }
        return 0xFF;
    }
    if (address < 0xB000) {
        // 0xA800-0xAFFF: extension area
        return 0xFF;
    }
    if (address < 0xC000) {
        // 0xB000-0xBFFF: cartridge ROM (4KB)
        if (state_.cartridge_inserted && !state_.cartridge_rom.empty()) {
            uint16_t cart_offset = address - 0xB000;
            if (cart_offset < state_.cartridge_rom.size())
                return state_.cartridge_rom[cart_offset];
        }
        return 0xFF;
    }
    if (address < 0xF000) {
        // 0xC000-0xEFFF: BASIC ROM (12KB)
        if (state_.cartridge_inserted && state_.cartridge_rom.size() > 0x1000) {
            uint16_t cart_offset = address - 0xB000;
            if (cart_offset < state_.cartridge_rom.size())
                return state_.cartridge_rom[cart_offset];
        }
        return state_.basic_rom[address - 0xC000];
    }
    // Monitor ROM: 0xF000-0xFFFF
    return state_.monitor_rom[address - 0xF000];
}

void MemorySystem::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        state_.video_ram[address + (state_.video_page << 13)] = value;
        return;
    }
    if (address < 0xA000) {
        state_.user_ram[address - 0x2000] = value;
        return;
    }
    if (address < 0xA800) {
        if (address == 0xA7C0) {
            // Gate array system register — direct write (masked to 0x5F)
            // Bit 0: video page select (0=pixel/forme, 1=color/fond)
            // Bits 1-4: border color
            // Bit 5: read-only (lightpen)
            // Bit 6: writable
            // Bit 7: read-only (cassette input)
            state_.gate_array_reg = value & 0x5F;
            state_.video_page = value & 0x01;
            return;
        }
        if (pia_ && address >= 0xA7C1 && address <= 0xA7C3) {
            // MO5 PIA register mapping (address lines swapped vs standard 6821):
            //   0xA7C1 → PIA reg 2 (Port B data/DDR — keyboard scan)
            //   0xA7C2 → PIA reg 1 (CRA)
            //   0xA7C3 → PIA reg 3 (CRB — vsync interrupt)
            static constexpr uint8_t mo5_pia_map[4] = {0, 2, 1, 3};
            pia_->write_register(mo5_pia_map[address & 0x03], value);
        }
        return;
    }
    // 0xA800-0xFFFF: ROM/extension, writes ignored
}

const uint8_t* MemorySystem::get_pixel_ram() const { return state_.video_ram + 0x2000; }  // page 1: forme/shape bitmaps
const uint8_t* MemorySystem::get_color_ram() const { return state_.video_ram; }           // page 0: fond/color attributes

void MemorySystem::set_video_page(uint8_t page) { state_.video_page = page & 0x01; }
uint8_t MemorySystem::get_video_page() const { return state_.video_page; }

void MemorySystem::set_pia(PIA* pia) { pia_ = pia; }
void MemorySystem::set_gate_array(GateArray* ga) { gate_array_ = ga; }

void MemorySystem::insert_cartridge() { state_.cartridge_inserted = true; }
void MemorySystem::remove_cartridge() { state_.cartridge_inserted = false; }
bool MemorySystem::has_cartridge() const { return state_.cartridge_inserted; }

MO5MemoryState MemorySystem::get_state() const { return state_; }
void MemorySystem::set_state(const MO5MemoryState& state) { state_ = state; }

} // namespace crayon
