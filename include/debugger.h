#ifndef CRAYON_DEBUGGER_H
#define CRAYON_DEBUGGER_H

#include "types.h"
#include <vector>
#include <string>
#include <cstdint>

namespace crayon {

class EmulatorCore;
struct CPU6809State;

struct Breakpoint {
    uint16 address;
    bool enabled;
    bool has_condition;
    std::string condition;
    bool condition_only;

    Breakpoint(uint16 addr, bool en = true)
        : address(addr), enabled(en), has_condition(false), condition_only(false) {}
    Breakpoint(uint16 addr, const std::string& cond, bool en = true)
        : address(addr), enabled(en), has_condition(true), condition(cond), condition_only(false) {}
    explicit Breakpoint(const std::string& cond, bool en = true)
        : address(0), enabled(en), has_condition(true), condition(cond), condition_only(true) {}
};

enum class DebuggerState { Running, Paused, StepMode, TraceMode };

enum class TraceLevel { Off, Minimal, Normal, Full };

struct FrameStats {
    uint64 total_cycles;
    uint64 frame_count;
    double fps;
    double average_cycles_per_frame;
};

class Debugger {
public:
    explicit Debugger(EmulatorCore* emulator);
    ~Debugger() = default;

    void add_breakpoint(uint16 address);
    void add_breakpoint(uint16 address, const std::string& condition);
    void add_breakpoint(const std::string& condition);
    void remove_breakpoint(uint16 address);
    void enable_breakpoint(uint16 address, bool enabled);
    void clear_all_breakpoints();
    bool check_breakpoint(uint16 address) const;
    bool check_condition_breakpoints() const;
    const std::vector<Breakpoint>& get_breakpoints() const { return breakpoints_; }

    void step();
    void continue_execution();
    void pause();
    bool is_paused() const { return state_ == DebuggerState::Paused; }
    DebuggerState get_state() const { return state_; }

    std::string dump_cpu_state() const;
    std::string dump_memory(uint16 start, uint16 end) const;
    std::string disassemble_at_pc(int lines_before = 5, int lines_after = 5) const;

    void enable_trace(bool enabled);
    void set_trace_level(TraceLevel level);
    TraceLevel get_trace_level() const { return trace_level_; }
    bool is_trace_enabled() const { return trace_level_ != TraceLevel::Off; }
    void log_instruction(uint64 current_cycles = 0);
    const std::vector<std::string>& get_trace_log() const { return trace_log_; }
    void clear_trace_log();

    void update_frame_stats(uint64 cycles);
    FrameStats get_frame_stats() const { return frame_stats_; }
    void reset_frame_stats();

private:
    EmulatorCore* emulator_;
    std::vector<Breakpoint> breakpoints_;
    DebuggerState state_;
    TraceLevel trace_level_;
    std::vector<std::string> trace_log_;
    FrameStats frame_stats_;

    bool evaluate_condition(const std::string& condition, const CPU6809State& cpu) const;
};

} // namespace crayon

#endif // CRAYON_DEBUGGER_H
