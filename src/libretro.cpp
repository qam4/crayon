#include "libretro.h"
#include "emulator_core.h"
#include "types.h"
#include <cstring>
#include <memory>

static std::unique_ptr<crayon::EmulatorCore> g_emulator;
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

RETRO_API void retro_init(void) {}
RETRO_API void retro_deinit(void) { g_emulator.reset(); }
RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_get_system_info(struct retro_system_info* info) {
    std::memset(info, 0, sizeof(*info));
    info->library_name = "Crayon";
    info->library_version = "0.1.0";
    info->valid_extensions = "k7|rom|bin";
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
    struct retro_log_callback logging;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
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
    input_poll_cb();
    g_emulator->run_frame();

    const crayon::uint32* fb = g_emulator->get_framebuffer();
    if (fb && video_cb)
        video_cb(fb, crayon::DISPLAY_WIDTH, crayon::DISPLAY_HEIGHT,
                 crayon::DISPLAY_WIDTH * sizeof(crayon::uint32));

    // Audio
    int16_t audio_buf[1600];  // ~48000/50 * 2 channels worth
    int samples = 48000 / 50;
    g_emulator->get_audio_buffer(audio_buf, samples);
    if (audio_batch_cb) audio_batch_cb(audio_buf, samples);
}

RETRO_API bool retro_load_game(const struct retro_game_info* game) {
    // Set pixel format
    unsigned pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);

    crayon::Configuration config;
    g_emulator = std::make_unique<crayon::EmulatorCore>(config);

    // Load system ROMs from system directory
    const char* system_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir) {
        std::string basic_path = std::string(system_dir) + "/mo5_basic.rom";
        std::string monitor_path = std::string(system_dir) + "/mo5_monitor.rom";
        g_emulator->load_basic_rom(basic_path);
        g_emulator->load_monitor_rom(monitor_path);
    }

    // Load cartridge/K7 from game info
    if (game && game->data && game->size > 0)
        g_emulator->load_cartridge(static_cast<const crayon::uint8*>(game->data), game->size);

    g_emulator->reset();
    return true;
}

RETRO_API bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t) { return false; }
RETRO_API void retro_unload_game(void) { g_emulator.reset(); }
RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_PAL; }

RETRO_API size_t retro_serialize_size(void) { return 0; }
RETRO_API bool retro_serialize(void*, size_t) { return false; }
RETRO_API bool retro_unserialize(const void*, size_t) { return false; }

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned, bool, const char*) {}

RETRO_API void* retro_get_memory_data(unsigned id) {
    if (!g_emulator) return nullptr;
    if (id == RETRO_MEMORY_VIDEO_RAM) {
        // Return pointer to video RAM via memory state
        // TODO: Expose raw pointer from MemorySystem
    }
    return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_VIDEO_RAM) return 0x4000;
    if (id == RETRO_MEMORY_SYSTEM_RAM) return 0x6000;
    return 0;
}
