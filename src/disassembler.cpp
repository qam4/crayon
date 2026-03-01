#include "disassembler.h"
#include <sstream>
#include <iomanip>

namespace crayon {

Instruction Disassembler::disassemble_instruction(uint16 address, const uint8* memory) {
    Instruction instr;
    instr.address = address;
    instr.opcode = memory[address];
    instr.size = 1;
    instr.cycles = 1;

    // TODO: Full 6809 instruction decoding
    // Stub: treat every opcode as unknown single-byte
    std::ostringstream oss;
    oss << "??? $" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(instr.opcode);
    instr.mnemonic = oss.str();
    instr.operand_text = "";

    return instr;
}

std::vector<Instruction> Disassembler::disassemble_range(const uint8* memory, uint16 start, uint16 end) {
    std::vector<Instruction> result;
    uint16 addr = start;
    while (addr <= end && addr >= start) {  // handle wrap
        auto instr = disassemble_instruction(addr, memory);
        result.push_back(instr);
        addr += instr.size;
    }
    return result;
}

std::string Disassembler::format_instruction(const Instruction& instr) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << instr.address
        << ": " << instr.mnemonic;
    if (!instr.operand_text.empty()) oss << " " << instr.operand_text;
    return oss.str();
}

} // namespace crayon
