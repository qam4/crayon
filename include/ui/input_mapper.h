#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include <SDL.h>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include "input_handler.h"

class ConfigManager;

enum class InputDevice { Keyboard, Joystick0, Joystick1 };

struct JoystickInput {
    int joystick_id = -1;
    int button = -1;
    int axis = -1;
    int axis_direction = 0;
    bool is_button() const { return button >= 0; }
    bool is_axis() const { return axis >= 0; }
};

struct JoystickInfo {
    int id;
    std::string name;
};

class InputMapper {
public:
    InputMapper();
    ~InputMapper();

    void set_keyboard_mapping(crayon::MO5Key mo5_key, SDL_Keycode host_key);
    SDL_Keycode get_keyboard_mapping(crayon::MO5Key mo5_key) const;

    void detect_joysticks();
    std::vector<JoystickInfo> get_connected_joysticks() const;

    void load_from_config(ConfigManager& config);
    void save_to_config(ConfigManager& config);

private:
    void init_default_mappings();
    std::map<crayon::MO5Key, SDL_Keycode> keyboard_mappings_;
    std::map<int, SDL_Joystick*> joysticks_;
    std::map<int, JoystickInfo> joystick_info_;
};

#endif // INPUT_MAPPER_H
