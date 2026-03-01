#include "debugger.h"
#include "emulator_core.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace crayon {

Debugger::Debugger(EmulatorCore* emulator)
    : emulator_(emulator), state_(DebuggerState::Running),
      trace_level_(TraceLevel::Off), frame_stats_{} {}

void Debugger::add_breakpoint(uint16 address) {
    breakpoints_.emplace_back(address);
}

void Debugger::add_breakpoint(uint16 address, const std::string& condition) {
    breakpoints_.emplace_back(address, condition);
}

void Debugger::add_breakpoint(const std::string& condition) {
    breakpoints_.emplace_back(condition);
}

void Debugger::remove_breakpoint(uint16 address) {
    breakpoints_.erase(
        std::remove_if(breakpoints_.begin(), breakpoints_.end(),
            [address](const Breakpoint& bp) { return bp.address == address && !bp.condition_only; }),
        breakpoints_.end());
}

void Debugger::enable_breakpoint(uint16 address, bool enabled) {
    for (auto& bp : breakpoints_) {
        if (bp.address == address) bp.enabled = enabled;
    }
}

void Debugger::clear_all_breakpoints() { breakpoints_.clear(); }

bool Debugger::check_breakpoint(uint16 address) const {
    for (const auto& bp : breakpoints_) {
        if (!bp.enabled || bp.condition_only) continue;
        if (bp.address == address) {
            if (!bp.has_condition) return true;
            if (evaluate_condition(bp.condition, emulator_->get_cpu_state())) return true;
        }
    }
    return false;
}

bool Debugger::check_condition_breakpoints() const {
    for (const auto& bp : breakpoints_) {
        if (!bp.enabled || !bp.condition_only) continue;
        if (evaluate_condition(bp.condition, emulator_->get_cpu_state())) return true;
    }
    return false;
}

void Debugger::step() {
    emulator_->step();
    state_ = DebuggerState::Paused;
}

void Debugger::continue_execution() { state_ = DebuggerState::Running; }
void Debugger::pause() { state_ = DebuggerState::Paused; }

std::string Debugger::dump_cpu_state() const {
    auto s = emulator_->get_cpu_state();
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0')
        << "PC=" << std::setw(4) << s.pc
        << " A=" << std::setw(2) << (int)s.a
        << " B=" << std::setw(2) << (int)s.b
        << " X=" << std::setw(4) << s.x
        << " Y=" << std::setw(4) << s.y
        << " U=" << std::setw(4) << s.u
        << " S=" << std::setw(4) << s.s
        << " DP=" << std::setw(2) << (int)s.dp
        << " CC=" << std::setw(2) << (int)s.cc;
    return oss.str();
}

std::string Debugger::dump_memory(uint16 start, uint16 end) const {
    // TODO: Implement memory dump via emulator memory access
    (void)start; (void)end;
    return "Memory dump not yet implemented";
}

std::string Debugger::disassemble_at_pc(int lines_before, int lines_after) const {
    (void)lines_before; (void)lines_after;
    return "Disassembly not yet implemented";
}

void Debugger::enable_trace(bool enabled) {
    trace_level_ = enabled ? TraceLevel::Normal : TraceLevel::Off;
}

void Debugger::set_trace_level(TraceLevel level) { trace_level_ = level; }

void Debugger::log_instruction(uint64 /*current_cycles*/) {
    if (trace_level_ == TraceLevel::Off) return;
    trace_log_.push_back(dump_cpu_state());
}

void Debugger::clear_trace_log() { trace_log_.clear(); }

void Debugger::update_frame_stats(uint64 cycles) {
    frame_stats_.total_cycles += cycles;
    frame_stats_.frame_count++;
    if (frame_stats_.frame_count > 0)
        frame_stats_.average_cycles_per_frame =
            static_cast<double>(frame_stats_.total_cycles) / frame_stats_.frame_count;
}

void Debugger::reset_frame_stats() { frame_stats_ = FrameStats{}; }

bool Debugger::evaluate_condition(const std::string& /*condition*/, const CPU6809State& /*cpu*/) const {
    // TODO: Implement condition expression parser for 6809 registers
    return false;
}

} // namespace crayon
