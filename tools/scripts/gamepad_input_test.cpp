#include <simulator/sim_direction_input.hpp>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

using namespace SIM::Input;

static void WriteConfig(const std::string &pad) {
    std::ofstream("sim_config.ini") << "[PAD]\n" << pad << '\n';
}

int main() {
    assert(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) == 0);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    const int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 2, 16, 1);
    assert(device >= 0);
    SDL_Joystick *joy = SDL_JoystickOpen(device);
    assert(joy);
    SIM_config_type config{};
    SIM_Config_LoadDefaults(&config);
    auto &pad = config.padSettings;
    assert(pad.joyAxisDeadzone == SIM_CONFIG_JOY_DEFAULT_DEADZONE);
    assert(pad.upJoyKeySecondary == (SIM_CONFIG_JOY_AXIS_MINUS_MASK | 1));
    assert(pad.downJoyKeySecondary == (SIM_CONFIG_JOY_AXIS_PLUS_MASK | 1));
    assert(pad.leftJoyKeySecondary == SIM_CONFIG_JOY_AXIS_MINUS_MASK);
    assert(pad.rightJoyKeySecondary == SIM_CONFIG_JOY_AXIS_PLUS_MASK);

    // Old configurations retain primary bindings and inherit stick defaults.
    WriteConfig("UpJoyKey=7\nJoyAxisDeadzone=8191");
    SIM_Config_LoadConfigFile(&config);
    assert(pad.upJoyKey == 7 && pad.upJoyKeySecondary == (SIM_CONFIG_JOY_AXIS_MINUS_MASK | 1));
    pad.upJoyKey = SIM_CONFIG_JOY_HAT_MASK | SDL_HAT_UP;
    pad.downJoyKey = SIM_CONFIG_JOY_HAT_MASK | SDL_HAT_DOWN;
    pad.leftJoyKey = SIM_CONFIG_JOY_HAT_MASK | SDL_HAT_LEFT;
    pad.rightJoyKey = SIM_CONFIG_JOY_HAT_MASK | SDL_HAT_RIGHT;
    pad.upJoyKeySecondary = SIM_CONFIG_JOY_AXIS_MINUS_MASK | 1;
    pad.downJoyKeySecondary = SIM_CONFIG_JOY_AXIS_PLUS_MASK | 1;
    pad.leftJoyKeySecondary = SIM_CONFIG_JOY_AXIS_MINUS_MASK;
    pad.rightJoyKeySecondary = SIM_CONFIG_JOY_AXIS_PLUS_MASK;
    auto axis = [&](int index, int value) {
        assert(SDL_JoystickSetVirtualAxis(joy, index, value) == 0);
        SDL_JoystickUpdate();
    };
    auto hat = [&](int value) {
        assert(SDL_JoystickSetVirtualHat(joy, 0, value) == 0);
        SDL_JoystickUpdate();
    };
    auto held = [&] { return JoystickDirections(joy, pad); };
    axis(0, 0); axis(1, 0); hat(SDL_HAT_CENTERED);
    assert(held() == 0);
    const unsigned bits[] = {0x40, 0x80, 0x20, 0x10};
    const int hats[] = {SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT};
    for (int i = 0; i < 4; ++i) {
        const int index = i < 2 ? 1 : 0;
        const int value = (i % 2 == 0) ? -20000 : 20000;
        hat(hats[i]); assert(held() == bits[i]);
        axis(index, value); assert(held() == bits[i]);
        hat(0); assert(held() == bits[i]); // D-pad release cannot cancel stick.
        hat(hats[i]); axis(index, 0); assert(held() == bits[i]); // Nor the reverse.
        assert((ApplyDirections(0xffff, bits[i], held()) & bits[i]) == 0);
        hat(0);
        assert((ApplyDirections(0xffff, bits[i], held()) & bits[i]) == 0); // Keyboard remains held.
        assert(ApplyDirections(0xffff, 0, held()) == 0xffff);
    }
    hat(SDL_HAT_RIGHTUP); assert(held() == 0x50); hat(0);
    axis(0, -32768); assert(held() == 0x20);
    axis(0, 32767); assert(held() == 0x10);
    axis(0, 8191); assert(held() == 0);
    axis(0, 8192); assert(held() == 0x10);
    axis(0, -8191); assert(held() == 0);
    axis(0, -8192); assert(held() == 0x20);
    axis(0, 1000); assert(held() == 0);
    pad.joyAxisDeadzone = 0; assert(held() == 0x10); // No extra axis event required.
    axis(0, 0); assert(held() == 0);
    axis(0, -1); assert(held() == 0x20);
    axis(0, 1); assert(held() == 0x10);
    pad.rightJoyKeySecondary = -1; assert(held() == 0); // Live clear / rebind.
    pad.rightJoyKeySecondary = 3;
    assert(SDL_JoystickSetVirtualButton(joy, 3, 1) == 0);
    SDL_JoystickUpdate(); assert(held() == 0x10); // D-pads exposed as buttons also work.
    assert(!JoystickBindingHeld(joy, -1, 0));
    assert(!JoystickBindingHeld(joy, SIM_CONFIG_JOY_AXIS_PLUS_MASK | 99, 0));
    assert(!JoystickBindingHeld(joy, 99, 0));
    assert((ApplyDirections(0xfffe, 0x40, 0x10) & 1) == 0); // Preserve non-direction bits.

    SIM_Config_SaveConfigFile(&config);
    SIM_config_type loaded{};
    SIM_Config_LoadDefaults(&loaded);
    SIM_Config_LoadConfigFile(&loaded);
    assert(loaded.padSettings.joyAxisDeadzone == 0);
    assert(loaded.padSettings.upJoyKeySecondary == pad.upJoyKeySecondary);
    assert(loaded.padSettings.downJoyKeySecondary == pad.downJoyKeySecondary);
    assert(loaded.padSettings.leftJoyKeySecondary == pad.leftJoyKeySecondary);
    assert(loaded.padSettings.rightJoyKeySecondary == 3);
    assert(loaded.padSettings.upJoyKey == pad.upJoyKey);
    for (const auto &value : {"bad", "10oops", "99999999999999999999999999"}) {
        WriteConfig(std::string("JoyAxisDeadzone=") + value);
        SIM_Config_LoadConfigFile(&loaded);
        assert(loaded.padSettings.joyAxisDeadzone == SIM_CONFIG_JOY_DEFAULT_DEADZONE);
    }
    WriteConfig("JoyAxisDeadzone=-4"); SIM_Config_LoadConfigFile(&loaded);
    assert(loaded.padSettings.joyAxisDeadzone == 0);
    WriteConfig("JoyAxisDeadzone=32767"); SIM_Config_LoadConfigFile(&loaded);
    assert(loaded.padSettings.joyAxisDeadzone == SIM_CONFIG_JOY_MAX_DEADZONE);
    WriteConfig("UpJoyKeySecondary=-1\nRightJoyKeySecondary=5");
    SIM_Config_LoadDefaults(&loaded);
    SIM_Config_LoadConfigFile(&loaded);
    assert(loaded.padSettings.upJoyKeySecondary == -1); // Explicitly cleared stays cleared.
    assert(loaded.padSettings.rightJoyKeySecondary == 5); // Custom mappings override defaults.
    SIM_Config_SaveConfigFile(&loaded);
    SIM_Config_LoadDefaults(&loaded);
    SIM_Config_LoadConfigFile(&loaded);
    assert(loaded.padSettings.upJoyKeySecondary == -1);
    assert(loaded.padSettings.rightJoyKeySecondary == 5);
    assert(SDL_JoystickDetachVirtual(device) == 0);
    assert(JoystickDirections(joy, pad) == 0);
    SDL_JoystickClose(joy);
    SDL_Quit();
    std::puts("Gamepad input: overlap, directions, deadzones, rebinding, disconnect and config checks passed.");
}
