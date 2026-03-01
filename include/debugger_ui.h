#ifndef CRAYON_DEBUGGER_UI_H
#define CRAYON_DEBUGGER_UI_H

#include "debugger.h"
#include <string>
#include <vector>
#include <functional>

namespace crayon {

using CommandHandler = std::function<void(const std::vector<std::string>&)>;

class DebuggerUI {
public:
    explicit DebuggerUI(Debugger* debugger);
    ~DebuggerUI() = default;

    void process_command(const std::string& command_line);
    void print_help();

    void display_cpu_state();
    void display_memory(uint16 start, uint16 end);
    void display_disassembly(int lines_before = 5, int lines_after = 5);
    void display_frame_stats();
    void display_breakpoints();
    void display_trace_log(int lines = 20);

private:
    Debugger* debugger_;

    void cmd_help(const std::vector<std::string>& args);
    void cmd_continue(const std::vector<std::string>& args);
    void cmd_step(const std::vector<std::string>& args);
    void cmd_break(const std::vector<std::string>& args);
    void cmd_delete(const std::vector<std::string>& args);
    void cmd_registers(const std::vector<std::string>& args);
    void cmd_memory(const std::vector<std::string>& args);
    void cmd_disassemble(const std::vector<std::string>& args);
    void cmd_trace(const std::vector<std::string>& args);

    std::vector<std::string> parse_command(const std::string& command_line);
    uint16 parse_address(const std::string& addr_str);
};

} // namespace crayon

#endif // CRAYON_DEBUGGER_UI_H
