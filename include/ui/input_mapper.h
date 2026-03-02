#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include <SDL.h>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include "input_handler.h"

class ConfigManager;
class TextRenderer;

enum class InputDevice { Keyboard, Joystick0, Joystick1 };

struct JoystickInput {
    int joystick_id = -1;
    int button = -1;
    int axis = -1;
    int axis_direction = 0; // -1 for negative, +1 for positive
    bool is_button() const { return button >= 0; }
    bool is_axis() const { return axis >= 0; }
};

struct JoystickInfo {
    int id;
    std::string name;
    int num_buttons;
    int num_axes;
};

class InputMapper {
public:
    InputMapper();
    ~InputMapper();

    // Keyboard mapping
    void set_keyboard_mapping(crayon::MO5Key mo5_key, SDL_Keycode host_key);
    SDL_Keycode get_keyboard_mapping(crayon::MO5Key mo5_key) const;
    void reset_keyboard_mappings();
    
    // Joystick mapping
    void set_joystick_button_mapping(crayon::MO5Key mo5_key, int joystick_id, int button);
    void set_joystick_axis_mapping(crayon::MO5Key mo5_key, int joystick_id, int axis, int direction);
    JoystickInput get_joystick_mapping(crayon::MO5Key mo5_key) const;
    
    // Joystick detection
    void detect_joysticks();
    std::vector<JoystickInfo> get_connected_joysticks() const;
    
    // Config persistence
    void load_from_config(ConfigManager& config);
    void save_to_config(ConfigManager& config);
    
    // UI rendering
    void render_mapping_ui(SDL_Renderer* renderer, TextRenderer* text_renderer);
    bool process_mapping_ui_input(SDL_Keycode key);
    void show_mapping_ui(bool show) { show_ui_ = show; }
    bool is_mapping_ui_visible() const { return show_ui_; }

private:
    void init_default_mappings();
    std::string mo5_key_to_string(crayon::MO5Key key) const;
    std::string sdl_keycode_to_string(SDL_Keycode key) const;
    
    std::map<crayon::MO5Key, SDL_Keycode> keyboard_mappings_;
    std::map<crayon::MO5Key, JoystickInput> joystick_mappings_;
    std::map<int, SDL_Joystick*> joysticks_;
    std::map<int, JoystickInfo> joystick_info_;
    
    // UI state
    bool show_ui_ = false;
    int selected_key_index_ = 0;
    bool waiting_for_input_ = false;
    crayon::MO5Key selected_mo5_key_ = crayon::MO5Key::ENTER;
};

#endif // INPUT_MAPPER_H
