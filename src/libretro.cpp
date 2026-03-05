#include "libretro.h"
#include "emulator_core.h"
#include "savestate.h"
#include "vkeyboard.h"
#include "types.h"
#include <cstring>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <algorithm>
#include <array>

// ---------------------------------------------------------------------------
// Static state
// ---------------------------------------------------------------------------
static std::unique_ptr<crayon::EmulatorCore> g_emulator;
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

static crayon::VirtualKeyboard g_vkb;
static bool g_select_prev = false;          // Edge detection for SELECT toggle
static int16_t g_stereo_buf[960 * 2];       // 960 stereo frames per PAL frame
static uint32_t g_vkb_framebuffer[crayon::DISPLAY_WIDTH * crayon::DISPLAY_HEIGHT];

// VKB key release scheduling
static bool g_vkb_key_pending_release = false;
static crayon::MO5Key g_vkb_pending_key = crayon::MO5Key::SPACE;
static bool g_vkb_shift_was_active = false;

// ---------------------------------------------------------------------------
// Core options
// ---------------------------------------------------------------------------
static struct retro_variable core_options[] = {
    { "crayon_cassette_mode", "Cassette Load Mode; Fast|Slow (Real-time)" },
    { "crayon_vkb_transparency", "Virtual Keyboard Transparency; Opaque|Semi-Transparent|Transparent" },
    { "crayon_vkb_position", "Virtual Keyboard Position; Bottom|Top" },
    { nullptr, nullptr }
};

// ---------------------------------------------------------------------------
// Input descriptors
// ---------------------------------------------------------------------------
static struct retro_input_descriptor input_descriptors[] = {
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Action / VKB Press" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Enter / VKB Shift" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "VKB Move Position" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Toggle Virtual Keyboard" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "STOP" },
    { 0, 0, 0, 0, nullptr }
};

// ---------------------------------------------------------------------------
// RETROK → MO5Key mapping table (AZERTY layout)
// ---------------------------------------------------------------------------
struct RetroKeyMapping {
    retro_key rk;
    crayon::MO5Key mk;
};

static const RetroKeyMapping g_key_map[] = {
    // Letters — AZERTY: physical QWERTY positions map to AZERTY letters
    // The retro_key values represent physical key positions (QWERTY layout)
    // We map them to MO5 AZERTY scancodes
    { RETROK_a,     crayon::MO5Key::Q },    // PC 'A' key → MO5 'Q' (AZERTY)
    { RETROK_b,     crayon::MO5Key::B },
    { RETROK_c,     crayon::MO5Key::C },
    { RETROK_d,     crayon::MO5Key::D },
    { RETROK_e,     crayon::MO5Key::E },
    { RETROK_f,     crayon::MO5Key::F },
    { RETROK_g,     crayon::MO5Key::G },
    { RETROK_h,     crayon::MO5Key::H },
    { RETROK_i,     crayon::MO5Key::I },
    { RETROK_j,     crayon::MO5Key::J },
    { RETROK_k,     crayon::MO5Key::K },
    { RETROK_l,     crayon::MO5Key::L },
    { RETROK_m,     crayon::MO5Key::SLASH },  // PC 'M' → MO5 'M' (scancode SLASH)
    { RETROK_n,     crayon::MO5Key::N },
    { RETROK_o,     crayon::MO5Key::O },
    { RETROK_p,     crayon::MO5Key::P },
    { RETROK_q,     crayon::MO5Key::A },    // PC 'Q' → MO5 'A' (AZERTY)
    { RETROK_r,     crayon::MO5Key::R },
    { RETROK_s,     crayon::MO5Key::S },
    { RETROK_t,     crayon::MO5Key::T },
    { RETROK_u,     crayon::MO5Key::U },
    { RETROK_v,     crayon::MO5Key::V },
    { RETROK_w,     crayon::MO5Key::Z },    // PC 'W' → MO5 'Z' (AZERTY)
    { RETROK_x,     crayon::MO5Key::X },
    { RETROK_y,     crayon::MO5Key::Y },
    { RETROK_z,     crayon::MO5Key::W },    // PC 'Z' → MO5 'W' (AZERTY)

    // Digits
    { RETROK_0,     crayon::MO5Key::Key0 },
    { RETROK_1,     crayon::MO5Key::Key1 },
    { RETROK_2,     crayon::MO5Key::Key2 },
    { RETROK_3,     crayon::MO5Key::Key3 },
    { RETROK_4,     crayon::MO5Key::Key4 },
    { RETROK_5,     crayon::MO5Key::Key5 },
    { RETROK_6,     crayon::MO5Key::Key6 },
    { RETROK_7,     crayon::MO5Key::Key7 },
    { RETROK_8,     crayon::MO5Key::Key8 },
    { RETROK_9,     crayon::MO5Key::Key9 },

    // Special keys
    { RETROK_RETURN,    crayon::MO5Key::ENTER },
    { RETROK_SPACE,     crayon::MO5Key::SPACE },
    { RETROK_BACKSPACE, crayon::MO5Key::ACC2 },
    { RETROK_TAB,       crayon::MO5Key::STOP },
    { RETROK_ESCAPE,    crayon::MO5Key::STOP },
    { RETROK_DELETE,    crayon::MO5Key::EFF },
    { RETROK_INSERT,    crayon::MO5Key::INS },
    { RETROK_HOME,      crayon::MO5Key::RAZ },

    // Arrow keys
    { RETROK_UP,    crayon::MO5Key::UP },
    { RETROK_DOWN,  crayon::MO5Key::DOWN },
    { RETROK_LEFT,  crayon::MO5Key::LEFT },
    { RETROK_RIGHT, crayon::MO5Key::RIGHT },

    // Modifiers
    { RETROK_LSHIFT, crayon::MO5Key::SHIFT },
    { RETROK_RSHIFT, crayon::MO5Key::BASIC },
    { RETROK_LCTRL,  crayon::MO5Key::CNT },

    // Punctuation
    { RETROK_COMMA,     crayon::MO5Key::M },      // PC ',' → MO5 ','
    { RETROK_PERIOD,    crayon::MO5Key::COMMA },   // PC '.' → MO5 '.'
    { RETROK_SEMICOLON, crayon::MO5Key::SLASH },   // PC ';' → MO5 'M'
    { RETROK_MINUS,     crayon::MO5Key::MINUS },
    { RETROK_EQUALS,    crayon::MO5Key::PLUS },
    { RETROK_ASTERISK,  crayon::MO5Key::STAR },
    { RETROK_SLASH,     crayon::MO5Key::DOT },
    { RETROK_AT,        crayon::MO5Key::AT },
    { RETROK_LEFTBRACKET,  crayon::MO5Key::STAR },
    { RETROK_RIGHTBRACKET, crayon::MO5Key::ACC },
};

static constexpr int KEY_MAP_SIZE = sizeof(g_key_map) / sizeof(g_key_map[0]);

// ---------------------------------------------------------------------------
// Helper: BIOS auto-discovery
// ---------------------------------------------------------------------------
static bool find_bios_file(const char* system_dir,
                           const char* const* filenames, int count,
                           std::string& out_path) {
    for (int i = 0; i < count; ++i) {
        // Search system directory root
        std::string path = std::string(system_dir) + "/" + filenames[i];
        std::ifstream f(path, std::ios::binary);
        if (f.good()) { out_path = path; return true; }

        // Search crayon subdirectory
        path = std::string(system_dir) + "/crayon/" + filenames[i];
        f.open(path, std::ios::binary);
        if (f.good()) { out_path = path; return true; }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Helper: get file extension (lowercase)
// ---------------------------------------------------------------------------
static std::string get_extension(const char* path) {
    if (!path) return "";
    std::string p(path);
    auto dot = p.rfind('.');
    if (dot == std::string::npos) return "";
    std::string ext = p.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

// ---------------------------------------------------------------------------
// Helper: RGBA → XRGB8888 conversion
// The emulator framebuffer uses RGBA (MO5_PALETTE_RGBA), but libretro
// expects XRGB8888. Convert: 0xRRGGBBAA → 0x00RRGGBB
// ---------------------------------------------------------------------------
static inline uint32_t rgba_to_xrgb(crayon::uint32 rgba) {
    uint32_t r = (rgba >> 24) & 0xFF;
    uint32_t g = (rgba >> 16) & 0xFF;
    uint32_t b = (rgba >>  8) & 0xFF;
    return (r << 16) | (g << 8) | b;
}

// ---------------------------------------------------------------------------
// Helper: poll core options and apply changes
// ---------------------------------------------------------------------------
static void poll_core_options() {
    bool updated = false;
    if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) || !updated)
        return;

    struct retro_variable var;

    // Cassette mode
    var.key = "crayon_cassette_mode";
    var.value = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (g_emulator) {
            if (std::strcmp(var.value, "Fast") == 0) {
                g_emulator->get_cassette().set_load_mode(crayon::CassetteLoadMode::Fast);
            } else {
                g_emulator->get_cassette().set_load_mode(crayon::CassetteLoadMode::Slow);
            }
        }
        if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] Cassette mode: %s\n", var.value);
    }

    // VKB transparency
    var.key = "crayon_vkb_transparency";
    var.value = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (std::strcmp(var.value, "Opaque") == 0)
            g_vkb.set_transparency(crayon::VKBTransparency::Opaque);
        else if (std::strcmp(var.value, "Semi-Transparent") == 0)
            g_vkb.set_transparency(crayon::VKBTransparency::SemiTransparent);
        else if (std::strcmp(var.value, "Transparent") == 0)
            g_vkb.set_transparency(crayon::VKBTransparency::Transparent);
        if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] VKB transparency: %s\n", var.value);
    }

    // VKB position
    var.key = "crayon_vkb_position";
    var.value = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (std::strcmp(var.value, "Bottom") == 0)
            g_vkb.set_position(crayon::VKBPosition::Bottom);
        else if (std::strcmp(var.value, "Top") == 0)
            g_vkb.set_position(crayon::VKBPosition::Top);
        if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] VKB position: %s\n", var.value);
    }
}

// ---------------------------------------------------------------------------
// Helper: process RetroPad input
// ---------------------------------------------------------------------------
static void process_retropad_input(bool suppress_keys = false) {
    if (!g_emulator) return;
    auto& input = g_emulator->get_input_handler();

    // SELECT toggles VKB (edge-detected) — always active
    bool select_now = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT) != 0;
    if (select_now && !g_select_prev)
        g_vkb.toggle_visible();
    g_select_prev = select_now;

    // Release pending VKB key from previous frame — always active
    if (g_vkb_key_pending_release) {
        input.set_key_state(g_vkb_pending_key, false);
        if (g_vkb_shift_was_active)
            input.set_key_state(crayon::MO5Key::SHIFT, false);
        g_vkb_key_pending_release = false;
    }

    // Skip RetroPad→MO5 key mapping when physical keyboard is active
    if (suppress_keys) return;

    if (g_vkb.is_visible()) {
        // VKB navigation mode
        // D-pad moves cursor (edge-detected would be better, but simple press works)
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
            g_vkb.move_cursor(0, -1);
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
            g_vkb.move_cursor(0, 1);
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
            g_vkb.move_cursor(-1, 0);
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
            g_vkb.move_cursor(1, 0);

        // B → press selected key
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) {
            crayon::MO5Key key = g_vkb.press_selected();
            bool shift = g_vkb.is_shift_active();
            if (shift)
                input.set_key_state(crayon::MO5Key::SHIFT, true);
            input.set_key_state(key, true);
            // Schedule release next frame
            g_vkb_key_pending_release = true;
            g_vkb_pending_key = key;
            g_vkb_shift_was_active = shift;
            // Clear shift after use
            if (shift) g_vkb.toggle_shift();
        }

        // A → toggle shift
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
            g_vkb.toggle_shift();

        // Y → toggle position
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y))
            g_vkb.toggle_position();
    } else {
        // Normal RetroPad mode: D-pad → arrows, B → SPACE, A → ENTER, Start → STOP
        input.set_key_state(crayon::MO5Key::UP,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) != 0);
        input.set_key_state(crayon::MO5Key::DOWN,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) != 0);
        input.set_key_state(crayon::MO5Key::LEFT,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) != 0);
        input.set_key_state(crayon::MO5Key::RIGHT,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) != 0);
        input.set_key_state(crayon::MO5Key::SPACE,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) != 0);
        input.set_key_state(crayon::MO5Key::ENTER,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) != 0);
        input.set_key_state(crayon::MO5Key::STOP,
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) != 0);
    }
}

// Keyboard hold timers: when a key is first pressed, hold it for N frames then release
static int g_kb_hold_timers[KEY_MAP_SIZE] = {};
static constexpr int KB_HOLD_FRAMES = 3;  // Hold key for 3 frames per press
static bool g_kb_prev_state[KEY_MAP_SIZE] = {};

// ---------------------------------------------------------------------------
// Helper: process physical keyboard
// ---------------------------------------------------------------------------
static bool process_keyboard_input() {
    if (!g_emulator) return false;
    auto& input = g_emulator->get_input_handler();
    bool any_pressed = false;

    for (int i = 0; i < KEY_MAP_SIZE; ++i) {
        bool pressed = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,
                                      static_cast<unsigned>(g_key_map[i].rk)) != 0;

        if (pressed) any_pressed = true;

        if (pressed && !g_kb_prev_state[i]) {
            input.set_key_state(g_key_map[i].mk, true);
            g_kb_hold_timers[i] = KB_HOLD_FRAMES;
        } else if (g_kb_hold_timers[i] > 0) {
            g_kb_hold_timers[i]--;
            if (g_kb_hold_timers[i] == 0) {
                input.set_key_state(g_key_map[i].mk, false);
            }
        } else if (!pressed && g_kb_prev_state[i]) {
            input.set_key_state(g_key_map[i].mk, false);
            g_kb_hold_timers[i] = 0;
        }

        g_kb_prev_state[i] = pressed;
    }
    return any_pressed;
}

// ---------------------------------------------------------------------------
// Helper: process light pen pointer
// ---------------------------------------------------------------------------
static void process_pointer_input() {
    if (!g_emulator) return;

    int16_t px = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t py = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    bool pressed = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED) != 0;

    // Convert [-32768, 32767] → [0, 319] x [0, 199]
    int x = static_cast<int>((static_cast<int32_t>(px) + 32768) * 319 / 65535);
    int y = static_cast<int>((static_cast<int32_t>(py) + 32768) * 199 / 65535);
    x = std::clamp(x, 0, 319);
    y = std::clamp(y, 0, 199);

    auto& lp = g_emulator->get_light_pen();
    lp.set_mouse_position(x, y, crayon::DISPLAY_WIDTH, crayon::DISPLAY_HEIGHT);
    lp.set_button_pressed(pressed);
}

// ===========================================================================
// Libretro API implementation
// ===========================================================================

RETRO_API void retro_init(void) {}

RETRO_API void retro_deinit(void) {
    g_emulator.reset();
    g_select_prev = false;
    g_vkb_key_pending_release = false;
}

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info* info) {
    std::memset(info, 0, sizeof(*info));
    info->library_name = "Crayon";
    info->library_version = "0.1.0";
    info->valid_extensions = "k7|rom|bin|mo5";
    info->need_fullpath = false;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info* info) {
    info->geometry.base_width = crayon::DISPLAY_WIDTH;
    info->geometry.base_height = crayon::DISPLAY_HEIGHT;
    info->geometry.max_width = crayon::DISPLAY_WIDTH;
    info->geometry.max_height = crayon::DISPLAY_HEIGHT;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = 50.0;
    info->timing.sample_rate = 48000.0;
}

RETRO_API void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    // Get log callback
    struct retro_log_callback logging;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;

    // Register core options
    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, core_options);

    // Input descriptors set in retro_load_game instead
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t) {}
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }
RETRO_API void retro_set_controller_port_device(unsigned, unsigned) {}

RETRO_API void retro_reset(void) {
    if (g_emulator) g_emulator->reset();
}

RETRO_API void retro_run(void) {
    if (!g_emulator) return;

    // 1. Poll input
    input_poll_cb();

    // 2. Check and apply core option changes
    poll_core_options();

    // 3. Process input: keyboard first, then RetroPad (skip key mapping if keyboard active)
    bool kb_active = process_keyboard_input();
    process_retropad_input(kb_active);
    process_pointer_input();

    // 4. Run one emulator frame
    g_emulator->run_frame();

    // 5. Video: get framebuffer, composite VKB if visible, submit
    const crayon::uint32* fb = g_emulator->get_framebuffer();
    if (fb && video_cb) {
        if (g_vkb.is_visible()) {
            // Copy and convert RGBA → XRGB8888, then composite VKB
            for (int i = 0; i < crayon::DISPLAY_WIDTH * crayon::DISPLAY_HEIGHT; ++i)
                g_vkb_framebuffer[i] = rgba_to_xrgb(fb[i]);
            g_vkb.render(g_vkb_framebuffer, crayon::DISPLAY_WIDTH, crayon::DISPLAY_HEIGHT);
            video_cb(g_vkb_framebuffer, crayon::DISPLAY_WIDTH, crayon::DISPLAY_HEIGHT,
                     crayon::DISPLAY_WIDTH * sizeof(uint32_t));
        } else {
            // Convert RGBA → XRGB8888 in-place copy
            for (int i = 0; i < crayon::DISPLAY_WIDTH * crayon::DISPLAY_HEIGHT; ++i)
                g_vkb_framebuffer[i] = rgba_to_xrgb(fb[i]);
            video_cb(g_vkb_framebuffer, crayon::DISPLAY_WIDTH, crayon::DISPLAY_HEIGHT,
                     crayon::DISPLAY_WIDTH * sizeof(uint32_t));
        }
    }

    // 6. Audio: mono → stereo conversion
    {
        constexpr int SAMPLES_PER_FRAME = 960;  // 48000 / 50
        int16_t mono_buf[SAMPLES_PER_FRAME];
        std::memset(mono_buf, 0, sizeof(mono_buf));

        // Read available mono samples
        size_t available = g_emulator->get_audio().samples_available();
        size_t to_read = std::min(available, static_cast<size_t>(SAMPLES_PER_FRAME));
        if (to_read > 0)
            g_emulator->get_audio_buffer(mono_buf, to_read);

        // Duplicate mono to stereo interleaved, pad with silence
        for (int i = 0; i < SAMPLES_PER_FRAME; ++i) {
            g_stereo_buf[i * 2]     = mono_buf[i];  // left
            g_stereo_buf[i * 2 + 1] = mono_buf[i];  // right
        }

        if (audio_batch_cb)
            audio_batch_cb(g_stereo_buf, SAMPLES_PER_FRAME);
    }
}

RETRO_API bool retro_load_game(const struct retro_game_info* game) {
    // Set pixel format
    unsigned pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);

    // Set input descriptors
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_descriptors);

    crayon::Configuration config;
    g_emulator = std::make_unique<crayon::EmulatorCore>(config);

    // --- BIOS auto-discovery ---
    const char* system_dir = nullptr;
    if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) || !system_dir) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] System directory not available\n");
        g_emulator.reset();
        return false;
    }

    // Search for BASIC ROM
    static const char* basic_names[] = { "basic5.rom", "mo5_basic.rom", "mo5.bas" };
    std::string basic_path;
    if (!find_bios_file(system_dir, basic_names, 3, basic_path)) {
        if (log_cb) log_cb(RETRO_LOG_ERROR,
            "[Crayon] BASIC ROM not found. Searched for: basic5.rom, mo5_basic.rom, mo5.bas "
            "in %s and %s/crayon/\n", system_dir, system_dir);
        g_emulator.reset();
        return false;
    }

    // Search for monitor ROM
    static const char* monitor_names[] = { "mo5.rom", "mo5_monitor.rom", "mo5.mon" };
    std::string monitor_path;
    if (!find_bios_file(system_dir, monitor_names, 3, monitor_path)) {
        if (log_cb) log_cb(RETRO_LOG_ERROR,
            "[Crayon] Monitor ROM not found. Searched for: mo5.rom, mo5_monitor.rom, mo5.mon "
            "in %s and %s/crayon/\n", system_dir, system_dir);
        g_emulator.reset();
        return false;
    }

    // Load BIOS ROMs
    auto result = g_emulator->load_basic_rom(basic_path);
    if (result.is_err()) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Failed to load BASIC ROM: %s\n",
                           result.error.c_str());
        g_emulator.reset();
        return false;
    }

    result = g_emulator->load_monitor_rom(monitor_path);
    if (result.is_err()) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Failed to load monitor ROM: %s\n",
                           result.error.c_str());
        g_emulator.reset();
        return false;
    }

    if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] BIOS loaded: %s, %s\n",
                       basic_path.c_str(), monitor_path.c_str());

    // --- K7 vs cartridge detection ---
    if (game && game->data && game->size > 0) {
        std::string ext = get_extension(game->path);
        const auto* data = static_cast<const crayon::uint8*>(game->data);
        size_t size = game->size;

        if (ext == ".k7") {
            // Cassette: load_k7 takes a file path, so we need to write to temp file
            // if the frontend provided data in memory (need_fullpath = false)
            if (game->path) {
                auto k7_result = g_emulator->get_cassette().load_k7(std::string(game->path));
                if (k7_result.is_err()) {
                    if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Failed to load K7: %s\n",
                                       k7_result.error.c_str());
                    g_emulator.reset();
                    return false;
                }
            } else {
                // Write to temp file for load_k7
                std::string tmp_path = std::string(system_dir) + "/crayon_temp.k7";
                {
                    std::ofstream tmp(tmp_path, std::ios::binary);
                    if (!tmp) {
                        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Cannot create temp K7 file\n");
                        g_emulator.reset();
                        return false;
                    }
                    tmp.write(reinterpret_cast<const char*>(data),
                              static_cast<std::streamsize>(size));
                }
                auto k7_result = g_emulator->get_cassette().load_k7(tmp_path);
                std::remove(tmp_path.c_str());
                if (k7_result.is_err()) {
                    if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Failed to load K7: %s\n",
                                       k7_result.error.c_str());
                    g_emulator.reset();
                    return false;
                }
            }
            g_emulator->play_cassette();
            if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] K7 loaded and playing\n");

        } else if (ext == ".rom" || ext == ".bin" || ext == ".mo5") {
            // Cartridge ROM
            auto cart_result = g_emulator->load_cartridge(data, size);
            if (cart_result.is_err()) {
                if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Failed to load cartridge: %s\n",
                                   cart_result.error.c_str());
                g_emulator.reset();
                return false;
            }
            if (log_cb) log_cb(RETRO_LOG_INFO, "[Crayon] Cartridge loaded (%zu bytes)\n", size);

        } else {
            // Unknown extension: inspect header for K7 magic bytes
            // K7 files typically start with leader bytes (0x01) or have block structure
            bool is_k7 = false;
            if (size >= 3) {
                // Check for K7 block header pattern: leader (0x01) followed by sync (0x3C5A)
                for (size_t i = 0; i + 2 < size && i < 256; ++i) {
                    if (data[i] == 0x01 && data[i+1] == 0x3C && data[i+2] == 0x5A) {
                        is_k7 = true;
                        break;
                    }
                }
            }

            if (is_k7) {
                if (log_cb) log_cb(RETRO_LOG_WARN,
                    "[Crayon] Unknown extension, detected K7 format by header\n");
                // Write to temp and load as K7
                std::string tmp_path = std::string(system_dir) + "/crayon_temp.k7";
                {
                    std::ofstream tmp(tmp_path, std::ios::binary);
                    tmp.write(reinterpret_cast<const char*>(data),
                              static_cast<std::streamsize>(size));
                }
                auto k7_result = g_emulator->get_cassette().load_k7(tmp_path);
                std::remove(tmp_path.c_str());
                if (k7_result.is_ok())
                    g_emulator->play_cassette();
                else if (log_cb)
                    log_cb(RETRO_LOG_ERROR, "[Crayon] K7 header detected but load failed: %s\n",
                           k7_result.error.c_str());
            } else {
                if (log_cb) log_cb(RETRO_LOG_WARN,
                    "[Crayon] Unknown extension '%s', falling back to cartridge\n",
                    ext.c_str());
                auto cart_result = g_emulator->load_cartridge(data, size);
                if (cart_result.is_err()) {
                    if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Cartridge fallback failed: %s\n",
                                       cart_result.error.c_str());
                }
            }
        }
    }

    // Apply initial core options
    poll_core_options();

    g_emulator->reset();
    return true;
}

RETRO_API bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t) {
    return false;
}

RETRO_API void retro_unload_game(void) { g_emulator.reset(); }
RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_PAL; }

// ---------------------------------------------------------------------------
// Save states — in-memory serialization via SaveStateManager buffer API
// ---------------------------------------------------------------------------
static constexpr size_t SAVE_STATE_SIZE = 524288;  // 512KB upper bound

RETRO_API size_t retro_serialize_size(void) {
    return SAVE_STATE_SIZE;
}

RETRO_API bool retro_serialize(void* data, size_t size) {
    if (!g_emulator || size < SAVE_STATE_SIZE) return false;

    // Collect state from all emulator components
    crayon::SaveState state;
    state.cpu_state = g_emulator->get_cpu().get_state();
    state.gate_array_state = g_emulator->get_gate_array().get_state();
    state.memory_state = g_emulator->get_memory().get_state();
    state.pia_state = g_emulator->get_pia().get_state();
    state.audio_state = g_emulator->get_audio().get_state();
    state.input_state = g_emulator->get_input_handler().get_state();
    state.light_pen_state = g_emulator->get_light_pen().get_state();
    state.cassette_state = g_emulator->get_cassette().get_state();
    state.frame_count = g_emulator->get_frame_count();

    auto result = crayon::SaveStateManager::serialize_to_buffer(state);
    if (result.is_err()) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Serialize failed: %s\n",
                           result.error.c_str());
        return false;
    }

    const auto& buf = *result.value;
    // We prefix the buffer with a 4-byte length so unserialize knows the real size
    if (buf.size() + 4 > size) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Serialized state too large: %zu > %zu\n",
                           buf.size() + 4, size);
        return false;
    }

    std::memset(data, 0, size);
    auto* out = static_cast<uint8_t*>(data);
    // Write actual data length as little-endian uint32 prefix
    uint32_t data_len = static_cast<uint32_t>(buf.size());
    out[0] = data_len & 0xFF;
    out[1] = (data_len >> 8) & 0xFF;
    out[2] = (data_len >> 16) & 0xFF;
    out[3] = (data_len >> 24) & 0xFF;
    std::memcpy(out + 4, buf.data(), buf.size());
    return true;
}

RETRO_API bool retro_unserialize(const void* data, size_t size) {
    if (!g_emulator || !data || size < 4) return false;

    const auto* in = static_cast<const uint8_t*>(data);
    // Read actual data length from little-endian uint32 prefix
    uint32_t data_len = uint32_t(in[0])
                      | (uint32_t(in[1]) << 8)
                      | (uint32_t(in[2]) << 16)
                      | (uint32_t(in[3]) << 24);

    if (data_len == 0 || data_len + 4 > size) return false;

    auto result = crayon::SaveStateManager::deserialize_from_buffer(in + 4, data_len);
    if (result.is_err()) {
        if (log_cb) log_cb(RETRO_LOG_ERROR, "[Crayon] Deserialize failed: %s\n",
                           result.error.c_str());
        return false;
    }

    const auto& state = *result.value;
    g_emulator->get_cpu().set_state(state.cpu_state);
    g_emulator->get_gate_array().set_state(state.gate_array_state);
    g_emulator->get_memory().set_state(state.memory_state);
    g_emulator->get_pia().set_state(state.pia_state);
    g_emulator->get_audio().set_state(state.audio_state);
    g_emulator->get_input_handler().set_state(state.input_state);
    g_emulator->get_light_pen().set_state(state.light_pen_state);
    g_emulator->get_cassette().set_state(state.cassette_state);
    return true;
}

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned, bool, const char*) {}

RETRO_API void* retro_get_memory_data(unsigned) { return nullptr; }

RETRO_API size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_VIDEO_RAM) return 0x4000;   // 16KB video RAM
    if (id == RETRO_MEMORY_SYSTEM_RAM) return 0x8000;   // 32KB user RAM
    return 0;
}
