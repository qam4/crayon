// Generic 6809 ROM disassembler
// Usage: disasm_roms <rom_file> <base_address_hex> [output_file]
// Example: disasm_roms roms/mo5.rom F000 debug/monitor_rom.asm
//          disasm_roms roms/basic5.rom C000 debug/basic_rom.asm
#include "disassembler.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using namespace crayon;

// MO5-specific known addresses for annotation
static const std::map<uint16_t, std::string> KNOWN_LABELS = {
    {0x0000, "VIDEO_RAM_WINDOW"},
    {0x2000, "USER_RAM_START"},
    {0xA7C0, "GATE_ARRAY_REG"},
    {0xA7C1, "PIA_PORT_B"},
    {0xA7C2, "PIA_CRA"},
    {0xA7C3, "PIA_CRB"},
    {0xB000, "CARTRIDGE_ROM"},
    {0xC000, "BASIC_ROM_START"},
    {0xF000, "MONITOR_ROM_START"},
    {0xFFF2, "VEC_SWI3"},
    {0xFFF4, "VEC_SWI2"},
    {0xFFF6, "VEC_FIRQ"},
    {0xFFF8, "VEC_IRQ"},
    {0xFFFA, "VEC_SWI"},
    {0xFFFC, "VEC_NMI"},
    {0xFFFE, "VEC_RESET"},
};

static std::string annotate(const Instruction& instr) {
    for (auto& [addr, label] : KNOWN_LABELS) {
        std::ostringstream hex;
        hex << "$" << std::uppercase << std::hex
            << std::setfill('0') << std::setw(4) << addr;
        if (instr.operand_text.find(hex.str()) != std::string::npos)
            return " ; " + label;
    }
    if (instr.mnemonic == "RTI") return " ; Return from interrupt";
    return "";
}

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <rom_file> <base_address_hex> [output_file]\n"
              << "  rom_file       - path to binary ROM file\n"
              << "  base_address   - hex address where ROM is mapped (e.g. F000, C000)\n"
              << "  output_file    - optional output path (default: debug/<rom_name>.asm)\n"
              << "\nExamples:\n"
              << "  " << prog << " roms/mo5.rom F000\n"
              << "  " << prog << " roms/basic5.rom C000 debug/basic.asm\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) { print_usage(argv[0]); return 1; }

    std::string rom_path = argv[1];
    uint16_t base_addr = static_cast<uint16_t>(std::stoul(argv[2], nullptr, 16));

    // Load ROM
    std::ifstream rom(rom_path, std::ios::binary | std::ios::ate);
    if (!rom) { std::cerr << "Cannot open: " << rom_path << "\n"; return 1; }
    size_t rom_size = rom.tellg();
    rom.seekg(0);

    // Build 64KB memory image with ROM at base_addr
    uint8_t mem[0x10000];
    memset(mem, 0xFF, sizeof(mem));
    rom.read(reinterpret_cast<char*>(mem + base_addr), rom_size);
    std::cout << rom_path << ": " << rom_size << " bytes at $"
              << std::hex << std::uppercase << std::setfill('0')
              << std::setw(4) << base_addr << "\n";

    // Determine output path
    std::string output_path;
    if (argc >= 4) {
        output_path = argv[3];
    } else {
        std::filesystem::create_directories("debug");
        auto stem = std::filesystem::path(rom_path).stem().string();
        output_path = "debug/" + stem + ".asm";
    }

    uint16_t end_addr = static_cast<uint16_t>(
        std::min((size_t)base_addr + rom_size - 1, (size_t)0xFFFF));

    std::ofstream out(output_path);
    if (!out) { std::cerr << "Cannot create: " << output_path << "\n"; return 1; }

    // Header
    out << "; " << rom_path << " disassembly\n";
    out << "; Base: $" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(4) << base_addr << " - $" << std::setw(4) << end_addr
        << " (" << std::dec << rom_size << " bytes)\n\n";

    // Print vectors if ROM covers $FFF0-$FFFF
    if (base_addr <= 0xFFF0 && end_addr >= 0xFFFF) {
        out << "; --- 6809 Interrupt Vectors ---\n";
        auto vec = [&](uint16_t addr, const char* name) {
            uint16_t target = (mem[addr] << 8) | mem[addr + 1];
            out << "; " << name << " ($" << std::hex << std::uppercase
                << std::setfill('0') << std::setw(4) << addr << ") = $"
                << std::setw(4) << target << "\n";
        };
        vec(0xFFF2, "SWI3 ");
        vec(0xFFF4, "SWI2 ");
        vec(0xFFF6, "FIRQ ");
        vec(0xFFF8, "IRQ  ");
        vec(0xFFFA, "SWI  ");
        vec(0xFFFC, "NMI  ");
        vec(0xFFFE, "RESET");
        out << "\n";
    }

    // Disassemble
    Disassembler disasm;
    auto instructions = disasm.disassemble_range(mem, base_addr, end_addr);
    for (auto& instr : instructions) {
        auto it = KNOWN_LABELS.find(instr.address);
        if (it != KNOWN_LABELS.end())
            out << "\n" << it->second << ":\n";

        out << std::hex << std::uppercase << std::setfill('0')
            << std::setw(4) << instr.address << "  ";

        std::ostringstream hx;
        hx << std::hex << std::uppercase << std::setfill('0')
           << std::setw(2) << (int)instr.opcode;
        for (auto b : instr.operand_bytes)
            hx << " " << std::setw(2) << (int)b;
        std::string hb = hx.str();
        out << hb;
        for (int i = hb.size(); i < 15; ++i) out << ' ';

        out << instr.mnemonic;
        if (!instr.operand_text.empty()) out << " " << instr.operand_text;
        out << annotate(instr) << "\n";
    }

    std::cout << "Written: " << output_path << "\n";
    return 0;
}
