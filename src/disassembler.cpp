#include "disassembler.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace crayon {

// Addressing modes
enum class AddrMode : uint8 {
    INH,        // Inherent (no operand)
    IMM8,       // Immediate 8-bit
    IMM16,      // Immediate 16-bit
    DIR,        // Direct page
    EXT,        // Extended (16-bit address)
    IDX,        // Indexed
    REL8,       // Relative 8-bit (short branch)
    REL16,      // Relative 16-bit (long branch)
    REG_PAIR,   // Register pair (TFR/EXG)
    REG_LIST_S, // Register list for PSHS/PULS
    REG_LIST_U, // Register list for PSHU/PULU
    ILLEGAL     // Undefined opcode
};

struct OpcodeInfo {
    const char* mnemonic;
    AddrMode mode;
    uint8 cycles;
};

// Page 1 opcode table (0x00-0xFF)
static const OpcodeInfo page1_opcodes[256] = {
    // 0x00-0x0F: Direct memory ops
    {"NEG",  AddrMode::DIR, 6},   // 0x00
    {"???",  AddrMode::ILLEGAL, 1}, // 0x01
    {"???",  AddrMode::ILLEGAL, 1}, // 0x02
    {"COM",  AddrMode::DIR, 6},   // 0x03
    {"LSR",  AddrMode::DIR, 6},   // 0x04
    {"???",  AddrMode::ILLEGAL, 1}, // 0x05
    {"ROR",  AddrMode::DIR, 6},   // 0x06
    {"ASR",  AddrMode::DIR, 6},   // 0x07
    {"ASL",  AddrMode::DIR, 6},   // 0x08
    {"ROL",  AddrMode::DIR, 6},   // 0x09
    {"DEC",  AddrMode::DIR, 6},   // 0x0A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x0B
    {"INC",  AddrMode::DIR, 6},   // 0x0C
    {"TST",  AddrMode::DIR, 6},   // 0x0D
    {"JMP",  AddrMode::DIR, 3},   // 0x0E
    {"CLR",  AddrMode::DIR, 6},   // 0x0F

    // 0x10-0x1F: Misc
    {"???",  AddrMode::ILLEGAL, 1}, // 0x10 (prefix - handled separately)
    {"???",  AddrMode::ILLEGAL, 1}, // 0x11 (prefix - handled separately)
    {"NOP",  AddrMode::INH, 2},   // 0x12
    {"SYNC", AddrMode::INH, 4},   // 0x13
    {"???",  AddrMode::ILLEGAL, 1}, // 0x14
    {"???",  AddrMode::ILLEGAL, 1}, // 0x15
    {"LBRA", AddrMode::REL16, 5}, // 0x16
    {"LBSR", AddrMode::REL16, 9}, // 0x17
    {"???",  AddrMode::ILLEGAL, 1}, // 0x18
    {"DAA",  AddrMode::INH, 2},   // 0x19
    {"ORCC", AddrMode::IMM8, 3},  // 0x1A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x1B
    {"ANDCC",AddrMode::IMM8, 3},  // 0x1C
    {"SEX",  AddrMode::INH, 2},   // 0x1D
    {"EXG",  AddrMode::REG_PAIR, 8}, // 0x1E
    {"TFR",  AddrMode::REG_PAIR, 6}, // 0x1F

    // 0x20-0x2F: Short branches
    {"BRA",  AddrMode::REL8, 3},  // 0x20
    {"BRN",  AddrMode::REL8, 3},  // 0x21
    {"BHI",  AddrMode::REL8, 3},  // 0x22
    {"BLS",  AddrMode::REL8, 3},  // 0x23
    {"BCC",  AddrMode::REL8, 3},  // 0x24
    {"BCS",  AddrMode::REL8, 3},  // 0x25
    {"BNE",  AddrMode::REL8, 3},  // 0x26
    {"BEQ",  AddrMode::REL8, 3},  // 0x27
    {"BVC",  AddrMode::REL8, 3},  // 0x28
    {"BVS",  AddrMode::REL8, 3},  // 0x29
    {"BPL",  AddrMode::REL8, 3},  // 0x2A
    {"BMI",  AddrMode::REL8, 3},  // 0x2B
    {"BGE",  AddrMode::REL8, 3},  // 0x2C
    {"BLT",  AddrMode::REL8, 3},  // 0x2D
    {"BGT",  AddrMode::REL8, 3},  // 0x2E
    {"BLE",  AddrMode::REL8, 3},  // 0x2F

    // 0x30-0x3F: Misc
    {"LEAX", AddrMode::IDX, 4},   // 0x30
    {"LEAY", AddrMode::IDX, 4},   // 0x31
    {"LEAS", AddrMode::IDX, 4},   // 0x32
    {"LEAU", AddrMode::IDX, 4},   // 0x33
    {"PSHS", AddrMode::REG_LIST_S, 5}, // 0x34
    {"PULS", AddrMode::REG_LIST_S, 5}, // 0x35
    {"PSHU", AddrMode::REG_LIST_U, 5}, // 0x36
    {"PULU", AddrMode::REG_LIST_U, 5}, // 0x37
    {"???",  AddrMode::ILLEGAL, 1}, // 0x38
    {"RTS",  AddrMode::INH, 5},   // 0x39
    {"ABX",  AddrMode::INH, 3},   // 0x3A
    {"RTI",  AddrMode::INH, 6},   // 0x3B
    {"CWAI", AddrMode::IMM8, 20}, // 0x3C
    {"MUL",  AddrMode::INH, 11},  // 0x3D
    {"???",  AddrMode::ILLEGAL, 1}, // 0x3E
    {"SWI",  AddrMode::INH, 19},  // 0x3F

    // 0x40-0x4F: Inherent A ops
    {"NEGA", AddrMode::INH, 2},   // 0x40
    {"???",  AddrMode::ILLEGAL, 1}, // 0x41
    {"???",  AddrMode::ILLEGAL, 1}, // 0x42
    {"COMA", AddrMode::INH, 2},   // 0x43
    {"LSRA", AddrMode::INH, 2},   // 0x44
    {"???",  AddrMode::ILLEGAL, 1}, // 0x45
    {"RORA", AddrMode::INH, 2},   // 0x46
    {"ASRA", AddrMode::INH, 2},   // 0x47
    {"ASLA", AddrMode::INH, 2},   // 0x48
    {"ROLA", AddrMode::INH, 2},   // 0x49
    {"DECA", AddrMode::INH, 2},   // 0x4A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x4B
    {"INCA", AddrMode::INH, 2},   // 0x4C
    {"TSTA", AddrMode::INH, 2},   // 0x4D
    {"???",  AddrMode::ILLEGAL, 1}, // 0x4E
    {"CLRA", AddrMode::INH, 2},   // 0x4F

    // 0x50-0x5F: Inherent B ops
    {"NEGB", AddrMode::INH, 2},   // 0x50
    {"???",  AddrMode::ILLEGAL, 1}, // 0x51
    {"???",  AddrMode::ILLEGAL, 1}, // 0x52
    {"COMB", AddrMode::INH, 2},   // 0x53
    {"LSRB", AddrMode::INH, 2},   // 0x54
    {"???",  AddrMode::ILLEGAL, 1}, // 0x55
    {"RORB", AddrMode::INH, 2},   // 0x56
    {"ASRB", AddrMode::INH, 2},   // 0x57
    {"ASLB", AddrMode::INH, 2},   // 0x58
    {"ROLB", AddrMode::INH, 2},   // 0x59
    {"DECB", AddrMode::INH, 2},   // 0x5A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x5B
    {"INCB", AddrMode::INH, 2},   // 0x5C
    {"TSTB", AddrMode::INH, 2},   // 0x5D
    {"???",  AddrMode::ILLEGAL, 1}, // 0x5E
    {"CLRB", AddrMode::INH, 2},   // 0x5F

    // 0x60-0x6F: Indexed memory ops
    {"NEG",  AddrMode::IDX, 6},   // 0x60
    {"???",  AddrMode::ILLEGAL, 1}, // 0x61
    {"???",  AddrMode::ILLEGAL, 1}, // 0x62
    {"COM",  AddrMode::IDX, 6},   // 0x63
    {"LSR",  AddrMode::IDX, 6},   // 0x64
    {"???",  AddrMode::ILLEGAL, 1}, // 0x65
    {"ROR",  AddrMode::IDX, 6},   // 0x66
    {"ASR",  AddrMode::IDX, 6},   // 0x67
    {"ASL",  AddrMode::IDX, 6},   // 0x68
    {"ROL",  AddrMode::IDX, 6},   // 0x69
    {"DEC",  AddrMode::IDX, 6},   // 0x6A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x6B
    {"INC",  AddrMode::IDX, 6},   // 0x6C
    {"TST",  AddrMode::IDX, 6},   // 0x6D
    {"JMP",  AddrMode::IDX, 3},   // 0x6E
    {"CLR",  AddrMode::IDX, 6},   // 0x6F

    // 0x70-0x7F: Extended memory ops
    {"NEG",  AddrMode::EXT, 7},   // 0x70
    {"???",  AddrMode::ILLEGAL, 1}, // 0x71
    {"???",  AddrMode::ILLEGAL, 1}, // 0x72
    {"COM",  AddrMode::EXT, 7},   // 0x73
    {"LSR",  AddrMode::EXT, 7},   // 0x74
    {"???",  AddrMode::ILLEGAL, 1}, // 0x75
    {"ROR",  AddrMode::EXT, 7},   // 0x76
    {"ASR",  AddrMode::EXT, 7},   // 0x77
    {"ASL",  AddrMode::EXT, 7},   // 0x78
    {"ROL",  AddrMode::EXT, 7},   // 0x79
    {"DEC",  AddrMode::EXT, 7},   // 0x7A
    {"???",  AddrMode::ILLEGAL, 1}, // 0x7B
    {"INC",  AddrMode::EXT, 7},   // 0x7C
    {"TST",  AddrMode::EXT, 7},   // 0x7D
    {"JMP",  AddrMode::EXT, 4},   // 0x7E
    {"CLR",  AddrMode::EXT, 7},   // 0x7F

    // 0x80-0x8F: Immediate A
    {"SUBA", AddrMode::IMM8, 2},  // 0x80
    {"CMPA", AddrMode::IMM8, 2},  // 0x81
    {"SBCA", AddrMode::IMM8, 2},  // 0x82
    {"SUBD", AddrMode::IMM16, 4}, // 0x83
    {"ANDA", AddrMode::IMM8, 2},  // 0x84
    {"BITA", AddrMode::IMM8, 2},  // 0x85
    {"LDA",  AddrMode::IMM8, 2},  // 0x86
    {"???",  AddrMode::ILLEGAL, 1}, // 0x87
    {"EORA", AddrMode::IMM8, 2},  // 0x88
    {"ADCA", AddrMode::IMM8, 2},  // 0x89
    {"ORA",  AddrMode::IMM8, 2},  // 0x8A
    {"ADDA", AddrMode::IMM8, 2},  // 0x8B
    {"CMPX", AddrMode::IMM16, 4}, // 0x8C
    {"BSR",  AddrMode::REL8, 7},  // 0x8D
    {"LDX",  AddrMode::IMM16, 3}, // 0x8E
    {"???",  AddrMode::ILLEGAL, 1}, // 0x8F

    // 0x90-0x9F: Direct A
    {"SUBA", AddrMode::DIR, 4},   // 0x90
    {"CMPA", AddrMode::DIR, 4},   // 0x91
    {"SBCA", AddrMode::DIR, 4},   // 0x92
    {"SUBD", AddrMode::DIR, 6},   // 0x93
    {"ANDA", AddrMode::DIR, 4},   // 0x94
    {"BITA", AddrMode::DIR, 4},   // 0x95
    {"LDA",  AddrMode::DIR, 4},   // 0x96
    {"STA",  AddrMode::DIR, 4},   // 0x97
    {"EORA", AddrMode::DIR, 4},   // 0x98
    {"ADCA", AddrMode::DIR, 4},   // 0x99
    {"ORA",  AddrMode::DIR, 4},   // 0x9A
    {"ADDA", AddrMode::DIR, 4},   // 0x9B
    {"CMPX", AddrMode::DIR, 6},   // 0x9C
    {"JSR",  AddrMode::DIR, 7},   // 0x9D
    {"LDX",  AddrMode::DIR, 5},   // 0x9E
    {"STX",  AddrMode::DIR, 5},   // 0x9F

    // 0xA0-0xAF: Indexed A
    {"SUBA", AddrMode::IDX, 4},   // 0xA0
    {"CMPA", AddrMode::IDX, 4},   // 0xA1
    {"SBCA", AddrMode::IDX, 4},   // 0xA2
    {"SUBD", AddrMode::IDX, 6},   // 0xA3
    {"ANDA", AddrMode::IDX, 4},   // 0xA4
    {"BITA", AddrMode::IDX, 4},   // 0xA5
    {"LDA",  AddrMode::IDX, 4},   // 0xA6
    {"STA",  AddrMode::IDX, 4},   // 0xA7
    {"EORA", AddrMode::IDX, 4},   // 0xA8
    {"ADCA", AddrMode::IDX, 4},   // 0xA9
    {"ORA",  AddrMode::IDX, 4},   // 0xAA
    {"ADDA", AddrMode::IDX, 4},   // 0xAB
    {"CMPX", AddrMode::IDX, 6},   // 0xAC
    {"JSR",  AddrMode::IDX, 7},   // 0xAD
    {"LDX",  AddrMode::IDX, 5},   // 0xAE
    {"STX",  AddrMode::IDX, 5},   // 0xAF

    // 0xB0-0xBF: Extended A
    {"SUBA", AddrMode::EXT, 5},   // 0xB0
    {"CMPA", AddrMode::EXT, 5},   // 0xB1
    {"SBCA", AddrMode::EXT, 5},   // 0xB2
    {"SUBD", AddrMode::EXT, 7},   // 0xB3
    {"ANDA", AddrMode::EXT, 5},   // 0xB4
    {"BITA", AddrMode::EXT, 5},   // 0xB5
    {"LDA",  AddrMode::EXT, 5},   // 0xB6
    {"STA",  AddrMode::EXT, 5},   // 0xB7
    {"EORA", AddrMode::EXT, 5},   // 0xB8
    {"ADCA", AddrMode::EXT, 5},   // 0xB9
    {"ORA",  AddrMode::EXT, 5},   // 0xBA
    {"ADDA", AddrMode::EXT, 5},   // 0xBB
    {"CMPX", AddrMode::EXT, 7},   // 0xBC
    {"JSR",  AddrMode::EXT, 8},   // 0xBD
    {"LDX",  AddrMode::EXT, 6},   // 0xBE
    {"STX",  AddrMode::EXT, 6},   // 0xBF

    // 0xC0-0xCF: Immediate B
    {"SUBB", AddrMode::IMM8, 2},  // 0xC0
    {"CMPB", AddrMode::IMM8, 2},  // 0xC1
    {"SBCB", AddrMode::IMM8, 2},  // 0xC2
    {"ADDD", AddrMode::IMM16, 4}, // 0xC3
    {"ANDB", AddrMode::IMM8, 2},  // 0xC4
    {"BITB", AddrMode::IMM8, 2},  // 0xC5
    {"LDB",  AddrMode::IMM8, 2},  // 0xC6
    {"???",  AddrMode::ILLEGAL, 1}, // 0xC7
    {"EORB", AddrMode::IMM8, 2},  // 0xC8
    {"ADCB", AddrMode::IMM8, 2},  // 0xC9
    {"ORB",  AddrMode::IMM8, 2},  // 0xCA
    {"ADDB", AddrMode::IMM8, 2},  // 0xCB
    {"LDD",  AddrMode::IMM16, 3}, // 0xCC
    {"???",  AddrMode::ILLEGAL, 1}, // 0xCD
    {"LDU",  AddrMode::IMM16, 3}, // 0xCE
    {"???",  AddrMode::ILLEGAL, 1}, // 0xCF

    // 0xD0-0xDF: Direct B
    {"SUBB", AddrMode::DIR, 4},   // 0xD0
    {"CMPB", AddrMode::DIR, 4},   // 0xD1
    {"SBCB", AddrMode::DIR, 4},   // 0xD2
    {"ADDD", AddrMode::DIR, 6},   // 0xD3
    {"ANDB", AddrMode::DIR, 4},   // 0xD4
    {"BITB", AddrMode::DIR, 4},   // 0xD5
    {"LDB",  AddrMode::DIR, 4},   // 0xD6
    {"STB",  AddrMode::DIR, 4},   // 0xD7
    {"EORB", AddrMode::DIR, 4},   // 0xD8
    {"ADCB", AddrMode::DIR, 4},   // 0xD9
    {"ORB",  AddrMode::DIR, 4},   // 0xDA
    {"ADDB", AddrMode::DIR, 4},   // 0xDB
    {"LDD",  AddrMode::DIR, 5},   // 0xDC
    {"STD",  AddrMode::DIR, 5},   // 0xDD
    {"LDU",  AddrMode::DIR, 5},   // 0xDE
    {"STU",  AddrMode::DIR, 5},   // 0xDF

    // 0xE0-0xEF: Indexed B
    {"SUBB", AddrMode::IDX, 4},   // 0xE0
    {"CMPB", AddrMode::IDX, 4},   // 0xE1
    {"SBCB", AddrMode::IDX, 4},   // 0xE2
    {"ADDD", AddrMode::IDX, 6},   // 0xE3
    {"ANDB", AddrMode::IDX, 4},   // 0xE4
    {"BITB", AddrMode::IDX, 4},   // 0xE5
    {"LDB",  AddrMode::IDX, 4},   // 0xE6
    {"STB",  AddrMode::IDX, 4},   // 0xE7
    {"EORB", AddrMode::IDX, 4},   // 0xE8
    {"ADCB", AddrMode::IDX, 4},   // 0xE9
    {"ORB",  AddrMode::IDX, 4},   // 0xEA
    {"ADDB", AddrMode::IDX, 4},   // 0xEB
    {"LDD",  AddrMode::IDX, 5},   // 0xEC
    {"STD",  AddrMode::IDX, 5},   // 0xED
    {"LDU",  AddrMode::IDX, 5},   // 0xEE
    {"STU",  AddrMode::IDX, 5},   // 0xEF

    // 0xF0-0xFF: Extended B
    {"SUBB", AddrMode::EXT, 5},   // 0xF0
    {"CMPB", AddrMode::EXT, 5},   // 0xF1
    {"SBCB", AddrMode::EXT, 5},   // 0xF2
    {"ADDD", AddrMode::EXT, 7},   // 0xF3
    {"ANDB", AddrMode::EXT, 5},   // 0xF4
    {"BITB", AddrMode::EXT, 5},   // 0xF5
    {"LDB",  AddrMode::EXT, 5},   // 0xF6
    {"STB",  AddrMode::EXT, 5},   // 0xF7
    {"EORB", AddrMode::EXT, 5},   // 0xF8
    {"ADCB", AddrMode::EXT, 5},   // 0xF9
    {"ORB",  AddrMode::EXT, 5},   // 0xFA
    {"ADDB", AddrMode::EXT, 5},   // 0xFB
    {"LDD",  AddrMode::EXT, 6},   // 0xFC
    {"STD",  AddrMode::EXT, 6},   // 0xFD
    {"LDU",  AddrMode::EXT, 6},   // 0xFE
    {"STU",  AddrMode::EXT, 6},   // 0xFF
};

// Page 2 (0x10 prefix) - sparse, use a lookup function
static bool lookup_page2(uint8 opcode, OpcodeInfo& info) {
    switch (opcode) {
        // Long branches (relative 16-bit)
        case 0x21: info = {"LBRN",  AddrMode::REL16, 5}; return true;
        case 0x22: info = {"LBHI",  AddrMode::REL16, 5}; return true;
        case 0x23: info = {"LBLS",  AddrMode::REL16, 5}; return true;
        case 0x24: info = {"LBCC",  AddrMode::REL16, 5}; return true;
        case 0x25: info = {"LBCS",  AddrMode::REL16, 5}; return true;
        case 0x26: info = {"LBNE",  AddrMode::REL16, 5}; return true;
        case 0x27: info = {"LBEQ",  AddrMode::REL16, 5}; return true;
        case 0x28: info = {"LBVC",  AddrMode::REL16, 5}; return true;
        case 0x29: info = {"LBVS",  AddrMode::REL16, 5}; return true;
        case 0x2A: info = {"LBPL",  AddrMode::REL16, 5}; return true;
        case 0x2B: info = {"LBMI",  AddrMode::REL16, 5}; return true;
        case 0x2C: info = {"LBGE",  AddrMode::REL16, 5}; return true;
        case 0x2D: info = {"LBLT",  AddrMode::REL16, 5}; return true;
        case 0x2E: info = {"LBGT",  AddrMode::REL16, 5}; return true;
        case 0x2F: info = {"LBLE",  AddrMode::REL16, 5}; return true;
        // SWI2
        case 0x3F: info = {"SWI2",  AddrMode::INH, 20}; return true;
        // CMPD
        case 0x83: info = {"CMPD",  AddrMode::IMM16, 5}; return true;
        case 0x93: info = {"CMPD",  AddrMode::DIR, 7}; return true;
        case 0xA3: info = {"CMPD",  AddrMode::IDX, 7}; return true;
        case 0xB3: info = {"CMPD",  AddrMode::EXT, 8}; return true;
        // CMPY
        case 0x8C: info = {"CMPY",  AddrMode::IMM16, 5}; return true;
        case 0x9C: info = {"CMPY",  AddrMode::DIR, 7}; return true;
        case 0xAC: info = {"CMPY",  AddrMode::IDX, 7}; return true;
        case 0xBC: info = {"CMPY",  AddrMode::EXT, 8}; return true;
        // LDY
        case 0x8E: info = {"LDY",   AddrMode::IMM16, 4}; return true;
        case 0x9E: info = {"LDY",   AddrMode::DIR, 6}; return true;
        case 0xAE: info = {"LDY",   AddrMode::IDX, 6}; return true;
        case 0xBE: info = {"LDY",   AddrMode::EXT, 7}; return true;
        // STY
        case 0x9F: info = {"STY",   AddrMode::DIR, 6}; return true;
        case 0xAF: info = {"STY",   AddrMode::IDX, 6}; return true;
        case 0xBF: info = {"STY",   AddrMode::EXT, 7}; return true;
        // LDS
        case 0xCE: info = {"LDS",   AddrMode::IMM16, 4}; return true;
        case 0xDE: info = {"LDS",   AddrMode::DIR, 6}; return true;
        case 0xEE: info = {"LDS",   AddrMode::IDX, 6}; return true;
        case 0xFE: info = {"LDS",   AddrMode::EXT, 7}; return true;
        // STS
        case 0xDF: info = {"STS",   AddrMode::DIR, 6}; return true;
        case 0xEF: info = {"STS",   AddrMode::IDX, 6}; return true;
        case 0xFF: info = {"STS",   AddrMode::EXT, 7}; return true;
        default: return false;
    }
}

// Page 3 (0x11 prefix) - very sparse
static bool lookup_page3(uint8 opcode, OpcodeInfo& info) {
    switch (opcode) {
        // SWI3
        case 0x3F: info = {"SWI3",  AddrMode::INH, 20}; return true;
        // CMPU
        case 0x83: info = {"CMPU",  AddrMode::IMM16, 5}; return true;
        case 0x93: info = {"CMPU",  AddrMode::DIR, 7}; return true;
        case 0xA3: info = {"CMPU",  AddrMode::IDX, 7}; return true;
        case 0xB3: info = {"CMPU",  AddrMode::EXT, 8}; return true;
        // CMPS
        case 0x8C: info = {"CMPS",  AddrMode::IMM16, 5}; return true;
        case 0x9C: info = {"CMPS",  AddrMode::DIR, 7}; return true;
        case 0xAC: info = {"CMPS",  AddrMode::IDX, 7}; return true;
        case 0xBC: info = {"CMPS",  AddrMode::EXT, 8}; return true;
        default: return false;
    }
}

// Index register names for indexed addressing
static const char* idx_reg_names[4] = {"X", "Y", "U", "S"};

// Register names for TFR/EXG
static const char* tfr_reg_name(uint8 code) {
    switch (code) {
        case 0:  return "D";
        case 1:  return "X";
        case 2:  return "Y";
        case 3:  return "U";
        case 4:  return "S";
        case 5:  return "PC";
        case 8:  return "A";
        case 9:  return "B";
        case 10: return "CC";
        case 11: return "DP";
        default: return "?";
    }
}

// Helper to format hex values
static std::string hex8(uint8 val) {
    std::ostringstream oss;
    oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(val);
    return oss.str();
}

static std::string hex16(uint16 val) {
    std::ostringstream oss;
    oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << static_cast<int>(val);
    return oss.str();
}

// Decode indexed addressing postbyte, returns operand text and extra bytes consumed
static std::string decode_indexed(const uint8* memory, uint16 addr, uint8& extra_bytes) {
    uint8 postbyte = memory[addr];
    extra_bytes = 0;

    // Bit 7 clear: 5-bit signed offset, no indirection
    if ((postbyte & 0x80) == 0) {
        uint8 reg = (postbyte >> 5) & 0x03;
        int8_t offset = static_cast<int8_t>(postbyte & 0x1F);
        // Sign-extend 5-bit value
        if (offset & 0x10) {
            offset |= static_cast<int8_t>(0xE0);
        }
        std::ostringstream oss;
        oss << static_cast<int>(offset) << "," << idx_reg_names[reg];
        return oss.str();
    }

    // Bit 7 set: use bits 0-4 for sub-mode
    uint8 reg = (postbyte >> 5) & 0x03;
    uint8 submode = postbyte & 0x1F;

    std::ostringstream oss;

    switch (submode) {
        case 0x00: // ,R+  (no indirect)
            oss << "," << idx_reg_names[reg] << "+";
            break;
        case 0x01: // ,R++ (no indirect)
            oss << "," << idx_reg_names[reg] << "++";
            break;
        case 0x11: // [,R++]
            oss << "[," << idx_reg_names[reg] << "++]";
            break;
        case 0x02: // ,-R (no indirect)
            oss << ",-" << idx_reg_names[reg];
            break;
        case 0x03: // ,--R (no indirect)
            oss << ",--" << idx_reg_names[reg];
            break;
        case 0x13: // [,--R]
            oss << "[,--" << idx_reg_names[reg] << "]";
            break;
        case 0x04: // ,R (zero offset)
            oss << "," << idx_reg_names[reg];
            break;
        case 0x14: // [,R]
            oss << "[," << idx_reg_names[reg] << "]";
            break;
        case 0x05: // B,R
            oss << "B," << idx_reg_names[reg];
            break;
        case 0x15: // [B,R]
            oss << "[B," << idx_reg_names[reg] << "]";
            break;
        case 0x06: // A,R
            oss << "A," << idx_reg_names[reg];
            break;
        case 0x16: // [A,R]
            oss << "[A," << idx_reg_names[reg] << "]";
            break;
        case 0x08: { // 8-bit offset,R
            extra_bytes = 1;
            int8_t off = static_cast<int8_t>(memory[(addr + 1) & 0xFFFF]);
            oss << static_cast<int>(off) << "," << idx_reg_names[reg];
            break;
        }
        case 0x18: { // [8-bit offset,R]
            extra_bytes = 1;
            int8_t off = static_cast<int8_t>(memory[(addr + 1) & 0xFFFF]);
            oss << "[" << static_cast<int>(off) << "," << idx_reg_names[reg] << "]";
            break;
        }
        case 0x09: { // 16-bit offset,R
            extra_bytes = 2;
            uint16 hi = memory[(addr + 1) & 0xFFFF];
            uint16 lo = memory[(addr + 2) & 0xFFFF];
            int16_t off = static_cast<int16_t>((hi << 8) | lo);
            oss << static_cast<int>(off) << "," << idx_reg_names[reg];
            break;
        }
        case 0x19: { // [16-bit offset,R]
            extra_bytes = 2;
            uint16 hi = memory[(addr + 1) & 0xFFFF];
            uint16 lo = memory[(addr + 2) & 0xFFFF];
            int16_t off = static_cast<int16_t>((hi << 8) | lo);
            oss << "[" << static_cast<int>(off) << "," << idx_reg_names[reg] << "]";
            break;
        }
        case 0x0B: // D,R
            oss << "D," << idx_reg_names[reg];
            break;
        case 0x1B: // [D,R]
            oss << "[D," << idx_reg_names[reg] << "]";
            break;
        case 0x0C: { // 8-bit offset,PCR
            extra_bytes = 1;
            int8_t off = static_cast<int8_t>(memory[(addr + 1) & 0xFFFF]);
            uint16 target = static_cast<uint16>((addr + 2 + off) & 0xFFFF);
            oss << hex16(target) << ",PCR";
            break;
        }
        case 0x1C: { // [8-bit offset,PCR]
            extra_bytes = 1;
            int8_t off = static_cast<int8_t>(memory[(addr + 1) & 0xFFFF]);
            uint16 target = static_cast<uint16>((addr + 2 + off) & 0xFFFF);
            oss << "[" << hex16(target) << ",PCR]";
            break;
        }
        case 0x0D: { // 16-bit offset,PCR
            extra_bytes = 2;
            uint16 hi = memory[(addr + 1) & 0xFFFF];
            uint16 lo = memory[(addr + 2) & 0xFFFF];
            int16_t off = static_cast<int16_t>((hi << 8) | lo);
            uint16 target = static_cast<uint16>((addr + 3 + off) & 0xFFFF);
            oss << hex16(target) << ",PCR";
            break;
        }
        case 0x1D: { // [16-bit offset,PCR]
            extra_bytes = 2;
            uint16 hi = memory[(addr + 1) & 0xFFFF];
            uint16 lo = memory[(addr + 2) & 0xFFFF];
            int16_t off = static_cast<int16_t>((hi << 8) | lo);
            uint16 target = static_cast<uint16>((addr + 3 + off) & 0xFFFF);
            oss << "[" << hex16(target) << ",PCR]";
            break;
        }
        case 0x1F: { // [extended indirect]
            extra_bytes = 2;
            uint16 hi = memory[(addr + 1) & 0xFFFF];
            uint16 lo = memory[(addr + 2) & 0xFFFF];
            uint16 ea = (hi << 8) | lo;
            oss << "[" << hex16(ea) << "]";
            break;
        }
        default:
            oss << "???";
            break;
    }

    return oss.str();
}

// Decode register list for PSHS/PULS
static std::string decode_reg_list_s(uint8 postbyte) {
    // Order: CC, A, B, DP, X, Y, U, PC
    static const char* names[] = {"CC", "A", "B", "DP", "X", "Y", "U", "PC"};
    std::ostringstream oss;
    bool first = true;
    for (int i = 0; i < 8; i++) {
        if (postbyte & (1 << i)) {
            if (!first) oss << ",";
            oss << names[i];
            first = false;
        }
    }
    return oss.str();
}

// Decode register list for PSHU/PULU
static std::string decode_reg_list_u(uint8 postbyte) {
    // Order: CC, A, B, DP, X, Y, S, PC
    static const char* names[] = {"CC", "A", "B", "DP", "X", "Y", "S", "PC"};
    std::ostringstream oss;
    bool first = true;
    for (int i = 0; i < 8; i++) {
        if (postbyte & (1 << i)) {
            if (!first) oss << ",";
            oss << names[i];
            first = false;
        }
    }
    return oss.str();
}

Instruction Disassembler::disassemble_instruction(uint16 address, const uint8* memory) {
    Instruction instr;
    instr.address = address;
    uint16 pc = address;

    uint8 opcode = memory[pc & 0xFFFF];
    instr.opcode = opcode;
    pc = (pc + 1) & 0xFFFF;

    OpcodeInfo info;
    uint8 prefix_size = 0;

    // Check for prefix pages
    if (opcode == 0x10) {
        prefix_size = 1;
        uint8 opcode2 = memory[pc & 0xFFFF];
        pc = (pc + 1) & 0xFFFF;
        if (!lookup_page2(opcode2, info)) {
            // Illegal page 2 opcode
            instr.mnemonic = "???";
            instr.operand_text = "";
            instr.size = 2;
            instr.cycles = 1;
            instr.operand_bytes.push_back(opcode2);
            return instr;
        }
    } else if (opcode == 0x11) {
        prefix_size = 1;
        uint8 opcode2 = memory[pc & 0xFFFF];
        pc = (pc + 1) & 0xFFFF;
        if (!lookup_page3(opcode2, info)) {
            // Illegal page 3 opcode
            instr.mnemonic = "???";
            instr.operand_text = "";
            instr.size = 2;
            instr.cycles = 1;
            instr.operand_bytes.push_back(opcode2);
            return instr;
        }
    } else {
        info = page1_opcodes[opcode];
    }

    instr.mnemonic = info.mnemonic;
    instr.cycles = info.cycles;

    // Decode operand based on addressing mode
    switch (info.mode) {
        case AddrMode::INH:
            instr.operand_text = "";
            instr.size = 1 + prefix_size;
            break;

        case AddrMode::IMM8: {
            uint8 val = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(val);
            instr.operand_text = "#" + hex8(val);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::IMM16: {
            uint8 hi = memory[pc & 0xFFFF];
            uint8 lo = memory[(pc + 1) & 0xFFFF];
            instr.operand_bytes.push_back(hi);
            instr.operand_bytes.push_back(lo);
            uint16 val = (static_cast<uint16>(hi) << 8) | lo;
            instr.operand_text = "#" + hex16(val);
            instr.size = 3 + prefix_size;
            break;
        }

        case AddrMode::DIR: {
            uint8 val = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(val);
            instr.operand_text = hex8(val);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::EXT: {
            uint8 hi = memory[pc & 0xFFFF];
            uint8 lo = memory[(pc + 1) & 0xFFFF];
            instr.operand_bytes.push_back(hi);
            instr.operand_bytes.push_back(lo);
            uint16 val = (static_cast<uint16>(hi) << 8) | lo;
            instr.operand_text = hex16(val);
            instr.size = 3 + prefix_size;
            break;
        }

        case AddrMode::IDX: {
            uint8 postbyte = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(postbyte);
            uint8 extra = 0;
            instr.operand_text = decode_indexed(memory, pc & 0xFFFF, extra);
            // Add extra operand bytes
            for (uint8 i = 0; i < extra; i++) {
                instr.operand_bytes.push_back(memory[(pc + 1 + i) & 0xFFFF]);
            }
            instr.size = 2 + extra + prefix_size;
            break;
        }

        case AddrMode::REL8: {
            uint8 val = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(val);
            int8_t offset = static_cast<int8_t>(val);
            uint16 target = static_cast<uint16>((pc + 1 + offset) & 0xFFFF);
            instr.operand_text = hex16(target);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::REL16: {
            uint8 hi = memory[pc & 0xFFFF];
            uint8 lo = memory[(pc + 1) & 0xFFFF];
            instr.operand_bytes.push_back(hi);
            instr.operand_bytes.push_back(lo);
            int16_t offset = static_cast<int16_t>((static_cast<uint16>(hi) << 8) | lo);
            uint16 target = static_cast<uint16>((pc + 2 + offset) & 0xFFFF);
            instr.operand_text = hex16(target);
            instr.size = 3 + prefix_size;
            break;
        }

        case AddrMode::REG_PAIR: {
            uint8 postbyte = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(postbyte);
            uint8 src = (postbyte >> 4) & 0x0F;
            uint8 dst = postbyte & 0x0F;
            instr.operand_text = std::string(tfr_reg_name(src)) + "," + tfr_reg_name(dst);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::REG_LIST_S: {
            uint8 postbyte = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(postbyte);
            instr.operand_text = decode_reg_list_s(postbyte);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::REG_LIST_U: {
            uint8 postbyte = memory[pc & 0xFFFF];
            instr.operand_bytes.push_back(postbyte);
            instr.operand_text = decode_reg_list_u(postbyte);
            instr.size = 2 + prefix_size;
            break;
        }

        case AddrMode::ILLEGAL:
        default:
            instr.mnemonic = "???";
            instr.operand_text = "";
            instr.size = 1;
            instr.cycles = 1;
            break;
    }

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
