// libretro API header
// Minimal subset needed for crayon (Thomson MO5) core
// Full spec: https://docs.libretro.com/

#ifndef LIBRETRO_H
#define LIBRETRO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
  #define RETRO_API __declspec(dllexport)
#else
  #define RETRO_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RETRO_API_VERSION 1

#define RETRO_DEVICE_NONE     0
#define RETRO_DEVICE_JOYPAD   1
#define RETRO_DEVICE_KEYBOARD 3

#define RETRO_DEVICE_ID_JOYPAD_B      0
#define RETRO_DEVICE_ID_JOYPAD_Y      1
#define RETRO_DEVICE_ID_JOYPAD_SELECT 2
#define RETRO_DEVICE_ID_JOYPAD_START  3
#define RETRO_DEVICE_ID_JOYPAD_UP     4
#define RETRO_DEVICE_ID_JOYPAD_DOWN   5
#define RETRO_DEVICE_ID_JOYPAD_LEFT   6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT  7
#define RETRO_DEVICE_ID_JOYPAD_A      8
#define RETRO_DEVICE_ID_JOYPAD_X      9
#define RETRO_DEVICE_ID_JOYPAD_L     10
#define RETRO_DEVICE_ID_JOYPAD_R     11

#define RETRO_PIXEL_FORMAT_XRGB8888 1
#define RETRO_PIXEL_FORMAT_RGB565   2

#define RETRO_MEMORY_SAVE_RAM   0
#define RETRO_MEMORY_SYSTEM_RAM 2
#define RETRO_MEMORY_VIDEO_RAM  3

#define RETRO_REGION_PAL  1

#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT      10
#define RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS  31
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY    9
#define RETRO_ENVIRONMENT_SET_VARIABLES          16
#define RETRO_ENVIRONMENT_GET_VARIABLE           15
#define RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE    17
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE      27

enum retro_log_level {
    RETRO_LOG_DEBUG = 0, RETRO_LOG_INFO, RETRO_LOG_WARN, RETRO_LOG_ERROR
};

typedef void (*retro_log_printf_t)(enum retro_log_level level, const char* fmt, ...);
struct retro_log_callback { retro_log_printf_t log; };

struct retro_system_info {
    const char* library_name;
    const char* library_version;
    const char* valid_extensions;
    bool need_fullpath;
    bool block_extract;
};

struct retro_game_info {
    const char* path;
    const void* data;
    size_t size;
    const char* meta;
};

struct retro_system_av_info {
    struct { unsigned base_width, base_height, max_width, max_height; float aspect_ratio; } geometry;
    struct { double fps, sample_rate; } timing;
};

struct retro_variable { const char* key; const char* value; };
struct retro_input_descriptor { unsigned port, device, index, id; const char* description; };

typedef bool (*retro_environment_t)(unsigned cmd, void* data);
typedef void (*retro_video_refresh_t)(const void* data, unsigned width, unsigned height, size_t pitch);
typedef void (*retro_audio_sample_t)(int16_t left, int16_t right);
typedef size_t (*retro_audio_sample_batch_t)(const int16_t* data, size_t frames);
typedef void (*retro_input_poll_t)(void);
typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);

RETRO_API void retro_init(void);
RETRO_API void retro_deinit(void);
RETRO_API unsigned retro_api_version(void);
RETRO_API void retro_get_system_info(struct retro_system_info* info);
RETRO_API void retro_get_system_av_info(struct retro_system_av_info* info);
RETRO_API void retro_set_environment(retro_environment_t);
RETRO_API void retro_set_video_refresh(retro_video_refresh_t);
RETRO_API void retro_set_audio_sample(retro_audio_sample_t);
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
RETRO_API void retro_set_input_poll(retro_input_poll_t);
RETRO_API void retro_set_input_state(retro_input_state_t);
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device);
RETRO_API void retro_reset(void);
RETRO_API void retro_run(void);
RETRO_API size_t retro_serialize_size(void);
RETRO_API bool retro_serialize(void* data, size_t size);
RETRO_API bool retro_unserialize(const void* data, size_t size);
RETRO_API bool retro_load_game(const struct retro_game_info* game);
RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info);
RETRO_API void retro_unload_game(void);
RETRO_API unsigned retro_get_region(void);
RETRO_API void retro_cheat_reset(void);
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char* code);
RETRO_API void* retro_get_memory_data(unsigned id);
RETRO_API size_t retro_get_memory_size(unsigned id);

#ifdef __cplusplus
}
#endif

#endif // LIBRETRO_H
