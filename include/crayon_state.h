#ifndef CRAYON_STATE_H
#define CRAYON_STATE_H

// Convenience header: includes all MO5 data model structs and state types.
//
// Individual state structs are defined in their respective component headers:
//   CPU6809State      — cpu6809.h
//   GateArrayState    — gate_array.h
//   MO5MemoryState    — memory_system.h
//   PIAState          — pia.h
//   AudioState        — audio_system.h
//   CassetteState     — cassette_interface.h
//   LightPenState     — light_pen.h
//   InputState        — input_handler.h
//   SaveState         — savestate.h

#include "cpu6809.h"
#include "gate_array.h"
#include "memory_system.h"
#include "pia.h"
#include "audio_system.h"
#include "cassette_interface.h"
#include "light_pen.h"
#include "input_handler.h"
#include "savestate.h"

#endif // CRAYON_STATE_H
