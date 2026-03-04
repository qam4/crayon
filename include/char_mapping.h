#ifndef CRAYON_CHAR_MAPPING_H
#define CRAYON_CHAR_MAPPING_H

#include "input_handler.h"

namespace crayon {

struct CharMapping {
    MO5Key key;
    bool shift = false;
};

// Map ASCII character to MO5 key (AZERTY layout, caps mode)
inline bool char_to_mo5(char c, CharMapping& out) {
    switch (c) {
        case 'A': case 'a': out = {MO5Key::Q, false}; return true;
        case 'B': case 'b': out = {MO5Key::B, false}; return true;
        case 'C': case 'c': out = {MO5Key::C, false}; return true;
        case 'D': case 'd': out = {MO5Key::D, false}; return true;
        case 'E': case 'e': out = {MO5Key::E, false}; return true;
        case 'F': case 'f': out = {MO5Key::F, false}; return true;
        case 'G': case 'g': out = {MO5Key::G, false}; return true;
        case 'H': case 'h': out = {MO5Key::H, false}; return true;
        case 'I': case 'i': out = {MO5Key::I, false}; return true;
        case 'J': case 'j': out = {MO5Key::J, false}; return true;
        case 'K': case 'k': out = {MO5Key::K, false}; return true;
        case 'L': case 'l': out = {MO5Key::L, false}; return true;
        case 'M': case 'm': out = {MO5Key::SLASH, false}; return true;
        case 'N': case 'n': out = {MO5Key::N, false}; return true;
        case 'O': case 'o': out = {MO5Key::O, false}; return true;
        case 'P': case 'p': out = {MO5Key::P, false}; return true;
        case 'Q': case 'q': out = {MO5Key::A, false}; return true;
        case 'R': case 'r': out = {MO5Key::R, false}; return true;
        case 'S': case 's': out = {MO5Key::S, false}; return true;
        case 'T': case 't': out = {MO5Key::T, false}; return true;
        case 'U': case 'u': out = {MO5Key::U, false}; return true;
        case 'V': case 'v': out = {MO5Key::V, false}; return true;
        case 'W': case 'w': out = {MO5Key::Z, false}; return true;
        case 'X': case 'x': out = {MO5Key::X, false}; return true;
        case 'Y': case 'y': out = {MO5Key::Y, false}; return true;
        case 'Z': case 'z': out = {MO5Key::W, false}; return true;
        case '0': out = {MO5Key::Key0, false}; return true;
        case '1': out = {MO5Key::Key1, false}; return true;
        case '2': out = {MO5Key::Key2, false}; return true;
        case '3': out = {MO5Key::Key3, false}; return true;
        case '4': out = {MO5Key::Key4, false}; return true;
        case '5': out = {MO5Key::Key5, false}; return true;
        case '6': out = {MO5Key::Key6, false}; return true;
        case '7': out = {MO5Key::Key7, false}; return true;
        case '8': out = {MO5Key::Key8, false}; return true;
        case '9': out = {MO5Key::Key9, false}; return true;
        case ' ':  out = {MO5Key::SPACE, false}; return true;
        case '\n': out = {MO5Key::ENTER, false}; return true;
        case ',':  out = {MO5Key::M, false}; return true;
        case '.':  out = {MO5Key::COMMA, false}; return true;
        case '"':  out = {MO5Key::Key2, true}; return true;
        case '-':  out = {MO5Key::MINUS, false}; return true;
        case '+':  out = {MO5Key::PLUS, false}; return true;
        case '*':  out = {MO5Key::STAR, false}; return true;
        case '/':  out = {MO5Key::SLASH, true}; return true;
        case '@':  out = {MO5Key::AT, false}; return true;
        default: return false;
    }
}

} // namespace crayon

#endif // CRAYON_CHAR_MAPPING_H
