#include "joystick.h"

#include <SDL2/SDL.h>
#include <limits.h>
#include <string.h>

static bool neutral_input;
void Joystick::setNeutralInput(bool enabled) { neutral_input = enabled; }

static bool ensure_joystick_subsystem()
{
    if ((SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK) != 0)
        return true;
    return SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0;
}

Joystick::Joystick(unsigned int device_index)
    : device_index_(device_index), device_(NULL)
{
}

Joystick::~Joystick()
{
    close();
}

unsigned int Joystick::deviceCount()
{
    int count;
    if (!ensure_joystick_subsystem())
        return 0;
    count = SDL_NumJoysticks();
    return count > 0 ? static_cast<unsigned int>(count) : 0;
}

bool Joystick::open()
{
    const int count = static_cast<int>(deviceCount());
    if (device_ != NULL)
        return true;
    if (device_index_ > static_cast<unsigned int>(INT_MAX) ||
        static_cast<int>(device_index_) >= count)
        return false;
    device_ = SDL_JoystickOpen(static_cast<int>(device_index_));
    return device_ != NULL;
}

void Joystick::close()
{
    if (device_ == NULL)
        return;
    SDL_JoystickClose(device_);
    device_ = NULL;
}

bool Joystick::poll(DIJOYSTATE2 *state)
{
    int button_count;
    int button;
    if (state == NULL)
        return false;
    memset(state, 0, sizeof(*state));
    if (neutral_input) return false;
    if (device_ == NULL || !SDL_JoystickGetAttached(device_))
        return false;

    SDL_JoystickUpdate();
    if (SDL_JoystickNumAxes(device_) > 0)
        state->lX = SDL_JoystickGetAxis(device_, 0);
    if (SDL_JoystickNumAxes(device_) > 1)
        state->lY = SDL_JoystickGetAxis(device_, 1);
    if (SDL_JoystickNumAxes(device_) > 2)
        state->lZ = SDL_JoystickGetAxis(device_, 2);

    button_count = SDL_JoystickNumButtons(device_);
    if (button_count > 128)
        button_count = 128;
    for (button = 0; button < button_count; ++button)
        state->rgbButtons[button] = SDL_JoystickGetButton(device_, button)
                                        ? (uint8_t)0x80 : (uint8_t)0;
    return true;
}
