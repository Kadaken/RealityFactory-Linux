#include "joystick.h"

#include <assert.h>

int main()
{
    DIJOYSTATE2 state;
    Joystick unavailable(Joystick::deviceCount());

    assert(!unavailable.open());
    assert(!unavailable.poll(&state));
    assert(state.lX == 0);
    assert(state.lY == 0);
    assert(state.lZ == 0);
    assert(state.rgbButtons[0] == 0);
    unavailable.close();
    return 0;
}
