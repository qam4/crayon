#ifndef UI_IMGUI_DEBUGGER_UI_H
#define UI_IMGUI_DEBUGGER_UI_H

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <SDL.h>

struct ImGuiContext;

namespace crayon {
    class Debugger;
    class EmulatorCore;
    class Disassembler;
    struct CPU6809State;
    struct MO5MemoryState;
}

enum class DisplayMode { Overlay, SplitScreen };

struct WatchExpression {
    enum class Type { MemoryAddress, Register };
    Type type;
    std::string expression;
    std::string label;
    crayon::uint16 last_value = 0;
    bool changed = false;
    crayon::uint16 evaluate(const crayon::CPU6809State& cpu, const crayon::MO5MemoryState& memory) const;
};

struct DisassemblyCache {
    std::unordered_map<crayon::uint16, std::string> cache;
    std::string get_disassembly(crayon::uint16 address, crayon::Disassembler* disasm, const crayon::uint8* memory);
    void invalidate();
};

class ImGuiDebuggerUI {
public:
    static constexpr size_t MAX_BREAKPOINTS = 100;
    static constexpr size_t MAX_WATCH_EXPRESSIONS = 50;

    ImGuiDebuggerUI(crayon::Debugger* debugger, crayon::EmulatorCore* emulator,
                    SDL_Window* window, SDL_Renderer* renderer);
    ~ImGuiDebuggerUI();

    bool initialize();
    void shutdown();
    void show();
    void hide();
    bool is_visible() const;
    void render();
    void process_event(const SDL_Event& event);
    void set_display_mode(DisplayMode mode);
    DisplayMode get_display_mode() const;
    ImGuiContext* get_context() const { return imgui_context_; }
    void save_state();
    void load_state();

private:
    void render_cpu_state_panel();
    void render_memory_panel();
    void render_breakpoints_panel();
    void render_disassembly_panel();
    void render_watch_panel();
    void render_controls_panel();

    crayon::Debugger* debugger_;
    crayon::EmulatorCore* emulator_;
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    bool visible_ = false;
    DisplayMode display_mode_ = DisplayMode::Overlay;
    ImGuiContext* imgui_context_ = nullptr;
    DisassemblyCache disasm_cache_;
};

#endif // UI_IMGUI_DEBUGGER_UI_H
