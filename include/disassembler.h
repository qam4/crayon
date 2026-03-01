#ifndef CRAYON_DISASSEMBLER_H
#define CRAYON_DISASSEMBLER_H

#include "types.h"
#include <string>
#include <vector>

namespace crayon {

struct Instruction {
    uint16 address;
    uint8 opcode;
    std::vector<uint8> operand_bytes;
    std::string mnemonic;
    std::string operand_text;
    uint8 cycles;
    uint8 size;  // 1-5 bytes for 6809
};

class Disassembler {
public:
    Disassembler() = default;
    ~Disassembler() = default;

    Instruction disassemble_instruction(uint16 address, const uint8* memory);
    std::vector<Instruction> disassemble_range(const uint8* memory, uint16 start, uint16 end);
    std::string format_instruction(const Instruction& instr);
};

} // namespace crayon

#endif // CRAYON_DISASSEMBLER_H
