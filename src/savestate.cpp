#include "savestate.h"
#include <fstream>
#include <cstring>
#include <numeric>

namespace crayon {

// Simple binary writer/reader helpers
namespace {

constexpr uint32_t MAGIC = 0x4D4F3543; // "MO5C"

class BinaryWriter {
public:
    void write_u8(uint8_t v) { data_.push_back(v); }
    void write_u16(uint16_t v) { write_u8(v & 0xFF); write_u8(v >> 8); }
    void write_u32(uint32_t v) { write_u16(v & 0xFFFF); write_u16(v >> 16); }
    void write_u64(uint64_t v) { write_u32(v & 0xFFFFFFFF); write_u32(v >> 32); }
    void write_i16(int16_t v) { write_u16(static_cast<uint16_t>(v)); }
    void write_bool(bool v) { write_u8(v ? 1 : 0); }
    void write_bytes(const uint8_t* p, size_t n) { data_.insert(data_.end(), p, p + n); }
    void write_vec(const std::vector<uint8_t>& v) {
        write_u32(static_cast<uint32_t>(v.size()));
        write_bytes(v.data(), v.size());
    }
    const std::vector<uint8_t>& data() const { return data_; }
private:
    std::vector<uint8_t> data_;
};

class BinaryReader {
public:
    explicit BinaryReader(const std::vector<uint8_t>& data) : data_(data) {}
    bool ok() const { return pos_ <= data_.size(); }
    uint8_t read_u8() { return pos_ < data_.size() ? data_[pos_++] : (fail(), uint8_t(0)); }
    uint16_t read_u16() { uint16_t lo = read_u8(); return lo | (uint16_t(read_u8()) << 8); }
    uint32_t read_u32() { uint32_t lo = read_u16(); return lo | (uint32_t(read_u16()) << 16); }
    uint64_t read_u64() { uint64_t lo = read_u32(); return lo | (uint64_t(read_u32()) << 32); }
    int16_t read_i16() { return static_cast<int16_t>(read_u16()); }
    bool read_bool() { return read_u8() != 0; }
    void read_bytes(uint8_t* p, size_t n) {
        for (size_t i = 0; i < n && ok(); ++i) p[i] = read_u8();
    }
    std::vector<uint8_t> read_vec() {
        uint32_t sz = read_u32();
        std::vector<uint8_t> v(sz);
        read_bytes(v.data(), sz);
        return v;
    }
    size_t pos() const { return pos_; }
private:
    void fail() { pos_ = data_.size() + 1; }
    const std::vector<uint8_t>& data_;
    size_t pos_ = 0;
};

void write_cpu(BinaryWriter& w, const CPU6809State& s) {
    w.write_u8(s.a); w.write_u8(s.b);
    w.write_u16(s.x); w.write_u16(s.y); w.write_u16(s.u); w.write_u16(s.s);
    w.write_u16(s.pc); w.write_u8(s.dp); w.write_u8(s.cc);
    w.write_u64(s.clock_cycles);
    w.write_bool(s.irq_line); w.write_bool(s.firq_line);
    w.write_bool(s.nmi_pending); w.write_bool(s.nmi_armed);
    w.write_bool(s.halted); w.write_bool(s.cwai_waiting);
}

CPU6809State read_cpu(BinaryReader& r) {
    CPU6809State s;
    s.a = r.read_u8(); s.b = r.read_u8();
    s.x = r.read_u16(); s.y = r.read_u16(); s.u = r.read_u16(); s.s = r.read_u16();
    s.pc = r.read_u16(); s.dp = r.read_u8(); s.cc = r.read_u8();
    s.clock_cycles = r.read_u64();
    s.irq_line = r.read_bool(); s.firq_line = r.read_bool();
    s.nmi_pending = r.read_bool(); s.nmi_armed = r.read_bool();
    s.halted = r.read_bool(); s.cwai_waiting = r.read_bool();
    return s;
}

void write_gate_array(BinaryWriter& w, const GateArrayState& s) {
    // Framebuffer is transient — skip it, it gets re-rendered
    w.write_u16(s.beam_x); w.write_u16(s.beam_y);
    w.write_u64(s.frame_number);
    w.write_bool(s.frame_complete); w.write_bool(s.vsync_flag);
    w.write_u8(s.border_color);
}

GateArrayState read_gate_array(BinaryReader& r) {
    GateArrayState s;
    s.beam_x = r.read_u16(); s.beam_y = r.read_u16();
    s.frame_number = r.read_u64();
    s.frame_complete = r.read_bool(); s.vsync_flag = r.read_bool();
    s.border_color = r.read_u8();
    return s;
}

void write_memory(BinaryWriter& w, const MO5MemoryState& s) {
    w.write_bytes(s.video_ram, sizeof(s.video_ram));
    w.write_bytes(s.user_ram, sizeof(s.user_ram));
    // ROMs are not saved — they're loaded from files
    w.write_bool(s.cartridge_inserted);
    w.write_bool(s.basic_rom_loaded);
    w.write_bool(s.monitor_rom_loaded);
    w.write_vec(s.cartridge_rom);
}

MO5MemoryState read_memory(BinaryReader& r) {
    MO5MemoryState s;
    r.read_bytes(s.video_ram, sizeof(s.video_ram));
    r.read_bytes(s.user_ram, sizeof(s.user_ram));
    s.cartridge_inserted = r.read_bool();
    s.basic_rom_loaded = r.read_bool();
    s.monitor_rom_loaded = r.read_bool();
    s.cartridge_rom = r.read_vec();
    return s;
}

void write_pia(BinaryWriter& w, const PIAState& s) {
    w.write_u8(s.dra); w.write_u8(s.ddra); w.write_u8(s.cra);
    w.write_u8(s.drb); w.write_u8(s.ddrb); w.write_u8(s.crb);
    w.write_u8(s.output_latch_a); w.write_u8(s.output_latch_b);
    w.write_u8(s.input_pins_a); w.write_u8(s.input_pins_b);
    w.write_bool(s.irqa1_flag); w.write_bool(s.irqa2_flag);
    w.write_bool(s.irqb1_flag); w.write_bool(s.irqb2_flag);
}

PIAState read_pia(BinaryReader& r) {
    PIAState s;
    s.dra = r.read_u8(); s.ddra = r.read_u8(); s.cra = r.read_u8();
    s.drb = r.read_u8(); s.ddrb = r.read_u8(); s.crb = r.read_u8();
    s.output_latch_a = r.read_u8(); s.output_latch_b = r.read_u8();
    s.input_pins_a = r.read_u8(); s.input_pins_b = r.read_u8();
    s.irqa1_flag = r.read_bool(); s.irqa2_flag = r.read_bool();
    s.irqb1_flag = r.read_bool(); s.irqb2_flag = r.read_bool();
    return s;
}

void write_audio(BinaryWriter& w, const AudioState& s) {
    w.write_bool(s.buzzer_state);
    w.write_u32(s.sample_accumulator);
    w.write_u32(s.host_sample_rate);
}

AudioState read_audio(BinaryReader& r) {
    AudioState s;
    s.buzzer_state = r.read_bool();
    s.sample_accumulator = r.read_u32();
    s.host_sample_rate = r.read_u32();
    return s;
}

void write_input(BinaryWriter& w, const InputState& s) {
    for (int i = 0; i < MO5_KEY_COUNT; ++i)
        w.write_bool(s.keys[i]);
}

InputState read_input(BinaryReader& r) {
    InputState s;
    for (int i = 0; i < MO5_KEY_COUNT; ++i)
        s.keys[i] = r.read_bool();
    return s;
}

void write_light_pen(BinaryWriter& w, const LightPenState& s) {
    w.write_i16(s.screen_x); w.write_i16(s.screen_y);
    w.write_bool(s.detected); w.write_bool(s.button_pressed); w.write_bool(s.strobe_active);
}

LightPenState read_light_pen(BinaryReader& r) {
    LightPenState s;
    s.screen_x = r.read_i16(); s.screen_y = r.read_i16();
    s.detected = r.read_bool(); s.button_pressed = r.read_bool(); s.strobe_active = r.read_bool();
    return s;
}

void write_cassette(BinaryWriter& w, const CassetteState& s) {
    w.write_vec(s.k7_data);
    w.write_u32(static_cast<uint32_t>(s.read_position));
    w.write_u8(s.bit_position);
    w.write_bool(s.playing); w.write_bool(s.recording);
    w.write_vec(s.record_buffer);
}

CassetteState read_cassette(BinaryReader& r) {
    CassetteState s;
    s.k7_data = r.read_vec();
    s.read_position = r.read_u32();
    s.bit_position = r.read_u8();
    s.playing = r.read_bool(); s.recording = r.read_bool();
    s.record_buffer = r.read_vec();
    return s;
}

// CRC32 (standard polynomial)
uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return ~crc;
}

} // anonymous namespace

Result<void> SaveStateManager::save(const std::string& path, const SaveState& state) {
    BinaryWriter w;

    // Header
    w.write_u32(MAGIC);
    w.write_u32(CURRENT_VERSION);

    // All component states
    write_cpu(w, state.cpu_state);
    write_gate_array(w, state.gate_array_state);
    write_memory(w, state.memory_state);
    write_pia(w, state.pia_state);
    write_audio(w, state.audio_state);
    write_input(w, state.input_state);
    write_light_pen(w, state.light_pen_state);
    write_cassette(w, state.cassette_state);
    w.write_u64(state.frame_count);

    // Checksum over all data so far
    uint32_t checksum = crc32(w.data().data(), w.data().size());
    w.write_u32(checksum);

    std::ofstream file(path, std::ios::binary);
    if (!file) return Result<void>::err("Cannot open file for writing: " + path);
    file.write(reinterpret_cast<const char*>(w.data().data()),
               static_cast<std::streamsize>(w.data().size()));
    if (!file) return Result<void>::err("Write failed: " + path);
    return Result<void>::ok();
}

Result<SaveState> SaveStateManager::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return Result<SaveState>::err("Cannot open file for reading: " + path);

    auto size = file.tellg();
    if (size < 12) return Result<SaveState>::err("File too small: " + path);
    file.seekg(0);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);
    if (!file) return Result<SaveState>::err("Read failed: " + path);

    // Verify checksum: last 4 bytes are the CRC32 of everything before them
    if (data.size() < 4) return Result<SaveState>::err("Corrupt save state");
    uint32_t stored_crc = uint32_t(data[data.size()-4])
                        | (uint32_t(data[data.size()-3]) << 8)
                        | (uint32_t(data[data.size()-2]) << 16)
                        | (uint32_t(data[data.size()-1]) << 24);
    uint32_t computed_crc = crc32(data.data(), data.size() - 4);
    if (stored_crc != computed_crc)
        return Result<SaveState>::err("Checksum mismatch in save state");

    BinaryReader r(data);

    uint32_t magic = r.read_u32();
    if (magic != MAGIC) return Result<SaveState>::err("Not a Crayon save state file");

    uint32_t version = r.read_u32();
    if (version != CURRENT_VERSION)
        return Result<SaveState>::err("Incompatible save state version");

    SaveState state;
    state.version = version;
    state.cpu_state = read_cpu(r);
    state.gate_array_state = read_gate_array(r);
    state.memory_state = read_memory(r);
    state.pia_state = read_pia(r);
    state.audio_state = read_audio(r);
    state.input_state = read_input(r);
    state.light_pen_state = read_light_pen(r);
    state.cassette_state = read_cassette(r);
    state.frame_count = r.read_u64();

    if (!r.ok()) return Result<SaveState>::err("Corrupt save state data");

    return Result<SaveState>::ok(state);
}

uint32 SaveStateManager::calculate_checksum(const SaveState& /*state*/) {
    return 0; // Checksum is computed over the serialized byte stream, not the struct
}

bool SaveStateManager::verify_checksum(const SaveState& /*state*/) {
    return true; // Verification happens during load() on the byte stream
}

} // namespace crayon
