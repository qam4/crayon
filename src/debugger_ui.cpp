#include "debugger_ui.h"
#include <iostream>
#include <sstream>

namespace crayon {

DebuggerUI::DebuggerUI(Debugger* debugger) : debugger_(debugger) {}

void DebuggerUI::process_command(const std::string& command_line) {
    auto args = parse_command(command_line);
    if (args.empty()) return;

    const auto& cmd = args[0];
    if (cmd == "help" || cmd == "h") cmd_help(args);
    else if (cmd == "continue" || cmd == "c") cmd_continue(args);
    else if (cmd == "step" || cmd == "s") cmd_step(args);
    else if (cmd == "break" || cmd == "b") cmd_break(args);
    else if (cmd == "delete" || cmd == "d") cmd_delete(args);
    else if (cmd == "registers" || cmd == "r") cmd_registers(args);
    else if (cmd == "memory" || cmd == "m") cmd_memory(args);
    else if (cmd == "disassemble" || cmd == "dis") cmd_disassemble(args);
    else if (cmd == "trace" || cmd == "t") cmd_trace(args);
    else std::cout << "Unknown command: " << cmd << ". Type 'help' for commands.\n";
}

void DebuggerUI::print_help() { cmd_help({}); }

void DebuggerUI::display_cpu_state() { std::cout << debugger_->dump_cpu_state() << "\n"; }

void DebuggerUI::display_memory(uint16 start, uint16 end) {
    std::cout << debugger_->dump_memory(start, end) << "\n";
}

void DebuggerUI::display_disassembly(int before, int after) {
    std::cout << debugger_->disassemble_at_pc(before, after) << "\n";
}

void DebuggerUI::display_frame_stats() {
    auto stats = debugger_->get_frame_stats();
    std::cout << "Frames: " << stats.frame_count
              << " Cycles: " << stats.total_cycles
              << " Avg cycles/frame: " << stats.average_cycles_per_frame << "\n";
}

void DebuggerUI::display_breakpoints() {
    const auto& bps = debugger_->get_breakpoints();
    if (bps.empty()) { std::cout << "No breakpoints set.\n"; return; }
    for (const auto& bp : bps) {
        std::cout << (bp.enabled ? "[*] " : "[ ] ");
        if (bp.condition_only) std::cout << "condition: " << bp.condition;
        else std::cout << std::hex << "0x" << bp.address;
        if (bp.has_condition && !bp.condition_only) std::cout << " if " << bp.condition;
        std::cout << "\n";
    }
}

void DebuggerUI::display_trace_log(int lines) {
    const auto& log = debugger_->get_trace_log();
    int start = static_cast<int>(log.size()) - lines;
    if (start < 0) start = 0;
    for (size_t i = start; i < log.size(); ++i)
        std::cout << log[i] << "\n";
}

void DebuggerUI::cmd_help(const std::vector<std::string>&) {
    std::cout << "Commands:\n"
              << "  help (h)          - Show this help\n"
              << "  continue (c)      - Continue execution\n"
              << "  step (s)          - Single step\n"
              << "  break (b) <addr>  - Set breakpoint\n"
              << "  delete (d) <addr> - Remove breakpoint\n"
              << "  registers (r)     - Show CPU registers\n"
              << "  memory (m) <start> <end> - Dump memory\n"
              << "  disassemble (dis) - Disassemble at PC\n"
              << "  trace (t) [on|off]- Toggle trace\n";
}

void DebuggerUI::cmd_continue(const std::vector<std::string>&) { debugger_->continue_execution(); }

void DebuggerUI::cmd_step(const std::vector<std::string>&) { debugger_->step(); display_cpu_state(); }

void DebuggerUI::cmd_break(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "Usage: break <address>\n"; return; }
    debugger_->add_breakpoint(parse_address(args[1]));
}

void DebuggerUI::cmd_delete(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "Usage: delete <address>\n"; return; }
    debugger_->remove_breakpoint(parse_address(args[1]));
}

void DebuggerUI::cmd_registers(const std::vector<std::string>&) { display_cpu_state(); }

void DebuggerUI::cmd_memory(const std::vector<std::string>& args) {
    if (args.size() < 3) { std::cout << "Usage: memory <start> <end>\n"; return; }
    display_memory(parse_address(args[1]), parse_address(args[2]));
}

void DebuggerUI::cmd_disassemble(const std::vector<std::string>&) { display_disassembly(); }

void DebuggerUI::cmd_trace(const std::vector<std::string>& args) {
    if (args.size() < 2) { std::cout << "Trace: " << (debugger_->is_trace_enabled() ? "on" : "off") << "\n"; return; }
    debugger_->enable_trace(args[1] == "on");
}

std::vector<std::string> DebuggerUI::parse_command(const std::string& command_line) {
    std::vector<std::string> tokens;
    std::istringstream iss(command_line);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

uint16 DebuggerUI::parse_address(const std::string& addr_str) {
    return static_cast<uint16>(std::stoul(addr_str, nullptr, 0));
}

} // namespace crayon
