#include "ui/imgui_debugger_ui.h"
#include "debugger.h"
#include "emulator_core.h"
#include "disassembler.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <cstdio>

// WatchExpression
crayon::uint16 WatchExpression::evaluate(const crayon::CPU6809State& cpu,
                                          const crayon::MO5MemoryState& /*memory*/) const {
    if (type == Type::Register) {
        if (expression == "A") return cpu.a;
        if (expression == "B") return cpu.b;
        if (expression == "D") return cpu.d();
        if (expression == "X") return cpu.x;
        if (expression == "Y") return cpu.y;
        if (expression == "U") return cpu.u;
        if (expression == "S") return cpu.s;
        if (expression == "PC") return cpu.pc;
        if (expression == "DP") return cpu.dp;
        if (expression == "CC") return cpu.cc;
    }
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
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);
    return true;
}

void ImGuiDebuggerUI::shutdown() {
    if (imgui_context_) {
        ImGui::SetCurrentContext(imgui_context_);
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }
}

void ImGuiDebuggerUI::show() { visible_ = true; }
void ImGuiDebuggerUI::hide() { visible_ = false; }
bool ImGuiDebuggerUI::is_visible() const { return visible_; }

void ImGuiDebuggerUI::render() {
    if (!visible_ || !imgui_context_) return;
    ImGui::SetCurrentContext(imgui_context_);
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    render_controls_panel();
    render_cpu_state_panel();
    render_disassembly_panel();
    render_memory_panel();

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiDebuggerUI::process_event(const SDL_Event& event) {
    if (!imgui_context_) return;
    ImGui::SetCurrentContext(imgui_context_);
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiDebuggerUI::set_display_mode(DisplayMode mode) { display_mode_ = mode; }
DisplayMode ImGuiDebuggerUI::get_display_mode() const { return display_mode_; }
void ImGuiDebuggerUI::save_state() {}
void ImGuiDebuggerUI::load_state() {}

void ImGuiDebuggerUI::render_controls_panel() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Controls")) {
        if (debugger_->is_paused()) {
            if (ImGui::Button("Continue")) debugger_->continue_execution();
            ImGui::SameLine();
            if (ImGui::Button("Step")) debugger_->step();
        } else {
            if (ImGui::Button("Pause")) debugger_->pause();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) emulator_->reset();
    }
    ImGui::End();
}

void ImGuiDebuggerUI::render_cpu_state_panel() {
    auto cpu = emulator_->get_cpu_state();
    auto pia = emulator_->get_pia_state();

    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CPU 6809E")) {
        ImGui::Text("A:%02X  B:%02X  D:%04X", cpu.a, cpu.b, cpu.d());
        ImGui::Text("X:%04X  Y:%04X", cpu.x, cpu.y);
        ImGui::Text("U:%04X  S:%04X", cpu.u, cpu.s);
        ImGui::Text("PC:%04X  DP:%02X", cpu.pc, cpu.dp);
        ImGui::Separator();
        ImGui::Text("CC:%02X  [%c%c%c%c%c%c%c%c]", cpu.cc,
            (cpu.cc & 0x80) ? 'E' : '-', (cpu.cc & 0x40) ? 'F' : '-',
            (cpu.cc & 0x20) ? 'H' : '-', (cpu.cc & 0x10) ? 'I' : '-',
            (cpu.cc & 0x08) ? 'N' : '-', (cpu.cc & 0x04) ? 'Z' : '-',
            (cpu.cc & 0x02) ? 'V' : '-', (cpu.cc & 0x01) ? 'C' : '-');
        ImGui::Text("Cycles: %llu", static_cast<unsigned long long>(cpu.clock_cycles));
        ImGui::Separator();
        ImGui::Text("PIA: DRA:%02X DRB:%02X", pia.dra, pia.drb);
        ImGui::Text("     CRA:%02X CRB:%02X", pia.cra, pia.crb);
        ImGui::Text("     DDRA:%02X DDRB:%02X", pia.ddra, pia.ddrb);
    }
    ImGui::End();
}

void ImGuiDebuggerUI::render_disassembly_panel() {
    ImGui::SetNextWindowPos(ImVec2(300, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 390), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Disassembly")) {
        auto cpu = emulator_->get_cpu_state();
        auto& mem = emulator_->get_memory();
        crayon::Disassembler disasm;

        // Show 20 instructions around PC
        uint16_t addr = cpu.pc;
        for (int i = 0; i < 20; ++i) {
            uint8_t buf[8];
            for (int j = 0; j < 8; ++j) buf[j] = mem.read(addr + j);
            auto instr = disasm.disassemble_instruction(addr, buf);
            auto text = disasm.format_instruction(instr);

            if (addr == cpu.pc)
                ImGui::TextColored(ImVec4(1, 1, 0, 1), ">>%04X: %s", addr, text.c_str());
            else
                ImGui::Text("  %04X: %s", addr, text.c_str());

            addr += instr.size;
        }
    }
    ImGui::End();
}

void ImGuiDebuggerUI::render_memory_panel() {
    static int view_addr = 0;
    ImGui::SetNextWindowPos(ImVec2(300, 410), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Memory")) {
        ImGui::InputInt("Address", &view_addr, 16, 256);
        if (view_addr < 0) view_addr = 0;
        if (view_addr > 0xFFFF) view_addr = 0xFFFF;

        // Region label
        uint16_t a = static_cast<uint16_t>(view_addr);
        const char* region = "Unknown";
        if (a < 0x2000) region = "Pixel RAM";
        else if (a < 0x4000) region = "Color RAM";
        else if (a < 0xA000) region = "User RAM";
        else if (a < 0xA800) region = "I/O (PIA)";
        else if (a < 0xC000) region = "Reserved";
        else if (a < 0xF000) region = "BASIC ROM";
        else region = "Monitor ROM";
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "[%s]", region);

        ImGui::Separator();
        auto& mem = emulator_->get_memory();
        for (int row = 0; row < 16; ++row) {
            uint16_t base = static_cast<uint16_t>(view_addr + row * 16);
            ImGui::Text("%04X:", base);
            ImGui::SameLine();
            for (int col = 0; col < 16; ++col) {
                uint16_t addr = base + col;
                ImGui::SameLine();
                ImGui::Text("%02X", mem.read(addr));
            }
            ImGui::SameLine();
            ImGui::Text(" ");
            ImGui::SameLine();
            for (int col = 0; col < 16; ++col) {
                uint8_t c = mem.read(base + col);
                ImGui::SameLine(0, 0);
                ImGui::Text("%c", (c >= 32 && c < 127) ? c : '.');
            }
        }
    }
    ImGui::End();
}

void ImGuiDebuggerUI::render_breakpoints_panel() {}
void ImGuiDebuggerUI::render_watch_panel() {}
