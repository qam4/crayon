#ifndef CRAYON_MEMORY_SYSTEM_H
#define CRAYON_MEMORY_SYSTEM_H

#include "types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace crayon {

class PIA;
class GateArray;
class CassetteInterface;

struct MO5MemoryState {
    // MO5 video RAM: two 8KB pages
    // Page 0 (0x0000-0x1FFF): fond/color attributes (fg/bg nibbles per 8-pixel block)
    // Page 1 (0x2000-0x3FFF): forme/shape pixel bitmaps
    // 0xA7C0 bit 0 selects which page is visible at CPU address 0x0000-0x1FFF:
    //   bit 0 = 0 → page 0 (fond/color)
    //   bit 0 = 1 → page 1 (forme/shape)
    uint8_t video_ram[0x4000] = {};     // 16KB: page0 (fond/color) + page1 (forme/shape)
    uint8_t user_ram[0x8000] = {};      // 32KB: 0x2000-0x9FFF
    uint8_t basic_rom[0x3000] = {};     // 12KB: 0xC000-0xEFFF
    uint8_t monitor_rom[0x1000] = {};   // 4KB: 0xF000-0xFFFF
    std::vector<uint8_t> cartridge_rom;
    bool cartridge_inserted = false;
    bool basic_rom_loaded = false;
    bool monitor_rom_loaded = false;
    uint8_t video_page = 0;             // 0=fond(color), 1=forme(shape) — bit 0 of gate array reg
    uint8_t gate_array_reg = 0;         // 0xA7C0 system register (bits 0-4,6 writable)
};

class MemorySystem {
public:
    MemorySystem();
    ~MemorySystem() = default;

    Result<void> load_basic_rom(const std::string& path);
    Result<void> load_basic_rom(const uint8_t* data, size_t size);
    Result<void> load_monitor_rom(const std::string& path);
    Result<void> load_monitor_rom(const uint8_t* data, size_t size);
    Result<void> load_cartridge(const std::string& path);
    Result<void> load_cartridge(const uint8_t* data, size_t size);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    const uint8_t* get_pixel_ram() const;
    const uint8_t* get_color_ram() const;

    void set_video_page(uint8_t page);
    uint8_t get_video_page() const;

    void set_pia(PIA* pia);
    void set_gate_array(GateArray* ga);
    void set_cassette(CassetteInterface* cass);

    void insert_cartridge();
    void remove_cartridge();
    bool has_cartridge() const;

    MO5MemoryState get_state() const;
    void set_state(const MO5MemoryState& state);

private:
    MO5MemoryState state_;
    PIA* pia_ = nullptr;
    GateArray* gate_array_ = nullptr;
    CassetteInterface* cassette_ = nullptr;
};

} // namespace crayon

#endif // CRAYON_MEMORY_SYSTEM_H
