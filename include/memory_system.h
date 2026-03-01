#ifndef CRAYON_MEMORY_SYSTEM_H
#define CRAYON_MEMORY_SYSTEM_H

#include "types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace crayon {

class PIA;
class GateArray;

struct MO5MemoryState {
    uint8_t video_ram[0x4000] = {};     // 16KB: pixel + color
    uint8_t user_ram[0x6000] = {};      // 24KB: 0x4000-0x9FFF
    uint8_t basic_rom[0x3000] = {};     // 12KB: 0xC000-0xEFFF
    uint8_t monitor_rom[0x1000] = {};   // 4KB: 0xF000-0xFFFF
    std::vector<uint8_t> cartridge_rom;
    bool cartridge_inserted = false;
    bool basic_rom_loaded = false;
    bool monitor_rom_loaded = false;
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

    void set_pia(PIA* pia);
    void set_gate_array(GateArray* ga);

    void insert_cartridge();
    void remove_cartridge();
    bool has_cartridge() const;

    MO5MemoryState get_state() const;
    void set_state(const MO5MemoryState& state);

private:
    MO5MemoryState state_;
    PIA* pia_ = nullptr;
    GateArray* gate_array_ = nullptr;
};

} // namespace crayon

#endif // CRAYON_MEMORY_SYSTEM_H
