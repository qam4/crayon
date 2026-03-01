#include "ui/imgui_debugger_ui.h"
#include "debugger.h"
#include "emulator_core.h"
#include "disassembler.h"

// WatchExpression
crayon::uint16 WatchExpression::evaluate(const crayon::CPU6809State& /*cpu*/,
                                          const crayon::MO5MemoryState& /*memory*/) const {
    // TODO: Parse expression and evaluate
    return 0;
}

// DisassemblyCache
std::string DisassemblyCache::get_disassembly(crayon::uint16 address,
                                               crayon::Disassembler* disasm,
                                               const crayon::uint8* memory) {
    auto it = cache.find(address);
    if (it != cache.end()) return it->second;
    if (!disasm || !memory) return "???";
    auto instr = disasm->disassemble_instruction(address, memory);
    auto text = disasm->format_instruction(instr);
    cache[address] = text;
    return text;
}

void DisassemblyCache::invalidate() { cache.clear(); }

// ImGuiDebuggerUI
ImGuiDebuggerUI::ImGuiDebuggerUI(crayon::Debugger* debugger, crayon::EmulatorCore* emulator,
                                   SDL_Window* window, SDL_Renderer* renderer)
    : debugger_(debugger), emulator_(emulator), window_(window), renderer_(renderer) {}

ImGuiDebuggerUI::~ImGuiDebuggerUI() { shutdown(); }

bool ImGuiDebuggerUI::initialize() {
    // TODO: Initialize ImGui context with SDL2 backend
    return true;
}

void ImGuiDebuggerUI::shutdown() {
    // TODO: Cleanup ImGui context
    imgui_context_ = nullptr;
}

void ImGuiDebuggerUI::show() { visible_ = true; }
void ImGuiDebuggerUI::hide() { visible_ = false; }
bool ImGuiDebuggerUI::is_visible() const { return visible_; }

void ImGuiDebuggerUI::render() {
    if (!visible_) return;
    // TODO: ImGui rendering — CPU state, memory, disassembly, breakpoints, watches
}

void ImGuiDebuggerUI::process_event(const SDL_Event& /*event*/) {
    // TODO: Forward to ImGui
}

void ImGuiDebuggerUI::set_display_mode(DisplayMode mode) { display_mode_ = mode; }
DisplayMode ImGuiDebuggerUI::get_display_mode() const { return display_mode_; }

void ImGuiDebuggerUI::save_state() {
    // TODO: Save debugger UI state to JSON
}

void ImGuiDebuggerUI::load_state() {
    // TODO: Load debugger UI state from JSON
}

void ImGuiDebuggerUI::render_cpu_state_panel() {}
void ImGuiDebuggerUI::render_memory_panel() {}
void ImGuiDebuggerUI::render_breakpoints_panel() {}
void ImGuiDebuggerUI::render_disassembly_panel() {}
void ImGuiDebuggerUI::render_watch_panel() {}
void ImGuiDebuggerUI::render_controls_panel() {}
