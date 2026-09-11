#include "joystick.h"
#include <SDL2/SDL.h>
#include <stdio.h>

int main()
{
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) return 1;
    const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 3, 2, 0);
    if (index < 0) { SDL_Quit(); return 1; }
    SDL_Joystick *control = SDL_JoystickOpen(index);
    bool ok = control != NULL;
    {
        Joystick device(index);
        DIJOYSTATE2 state = {};
        ok = ok && device.open();
        ok = ok && SDL_JoystickSetVirtualAxis(control, 0, 32767) == 0;
        ok = ok && SDL_JoystickSetVirtualButton(control, 0, 1) == 0;
        ok = ok && device.poll(&state) && state.lX == 32767 && state.rgbButtons[0];
        Joystick::setNeutralInput(true);
        ok = !device.poll(&state) && !state.lX && !state.lY && !state.lZ && !state.rgbButtons[0] && ok;
        Joystick::setNeutralInput(false);
        ok = device.poll(&state) && state.lX == 32767 && state.rgbButtons[0] && ok;
    }
    if (control) SDL_JoystickClose(control);
    SDL_JoystickDetachVirtual(index);
    SDL_Quit();
    fprintf(stderr, "joystick neutral boundary: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
