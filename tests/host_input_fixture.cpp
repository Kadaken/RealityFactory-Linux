#include "host_window.h"
#include "input_compat.h"
#include <SDL2/SDL.h>
#include <stdio.h>
static int ticks;
static LRESULT callback(HWND, UINT message, WPARAM, LPARAM)
{
    if (message == WM_TIMER) ++ticks;
    return 0;
}
static bool require(bool condition, const char *text)
{
    if (!condition) fprintf(stderr, "host/input: %s\n", text);
    return condition;
}
int main(void)
{
    RF_HostConfigure(0);
    HWND window = RF_HostCreate("Neutral host/input", 320, 240, callback);
    if (!window) return 1;
    bool ok = require(RF_HostCreate("second", 320, 240, callback) == NULL, "second window accepted");
    ok = require(!ShowWindow(window, SW_SHOWNORMAL) &&
        (SDL_GetWindowFlags((SDL_Window *)window) & SDL_WINDOW_HIDDEN), "hidden policy failed") && ok;
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_KEYDOWN; event.key.keysym.scancode = SDL_SCANCODE_W;
    SDL_PushEvent(&event);
    RF_HostPump();
    BYTE state[256]; WORD letter = 0;
    ok = require(GetAsyncKeyState('W') != 0 && GetKeyboardState(state) &&
                 ToAscii('W', 0, state, &letter, 0) == 1 && letter == 'w', "W hold/ASCII failed") && ok;
    event.key.keysym.scancode = SDL_SCANCODE_RSHIFT;
    SDL_PushEvent(&event); RF_HostPump();
    GetKeyboardState(state);
    ToAscii('W', 0, state, &letter, 0);
    ok = require(GetAsyncKeyState(VK_SHIFT) != 0 && letter == 'W', "right shift failed") && ok;
    event.type = SDL_KEYUP; event.key.keysym.scancode = SDL_SCANCODE_W;
    SDL_PushEvent(&event); // GetAsyncKeyState must pump legacy release-wait loops.
    ok = require(GetAsyncKeyState('W') == 0, "W release failed") && ok;
    SDL_zero(event); event.type = SDL_MOUSEBUTTONDOWN; event.button.button = SDL_BUTTON_LEFT;
    SDL_PushEvent(&event); RF_HostPump();
    ok = require(GetAsyncKeyState(VK_LBUTTON) != 0, "mouse button failed") && ok;
    SDL_zero(event); event.type = SDL_WINDOWEVENT; event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    SDL_PushEvent(&event); RF_HostPump();
    ok = require(!GetAsyncKeyState(VK_SHIFT) && !GetAsyncKeyState(VK_LBUTTON), "focus-loss reset failed") && ok;
    SDL_zero(event); event.type = SDL_MOUSEWHEEL; event.wheel.y = -2;
    RF_InputEvent(&event);
    ok = require(RF_InputTakeWheelStep() == -1 && RF_InputTakeWheelStep() == -1 &&
        RF_InputTakeWheelStep() == 0, "wheel detents were not preserved") && ok;
    SDL_zero(event); event.type = SDL_MOUSEWHEEL; event.wheel.y = 1;
    event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    RF_InputEvent(&event);
    ok = require(RF_InputTakeWheelStep() == -1 && RF_InputTakeWheelStep() == 0,
        "flipped wheel direction failed") && ok;
    GetKeyboardState(state);
    ok = require(ToAscii(0xBA, 0, state, &letter, 0) == 1 && letter == ';', "ASCII punctuation failed") && ok;
    ok = require(SetTimer(window, 0, 10, NULL) == 1, "timer setup failed") && ok;
    SDL_Delay(35); RF_HostPump();
    ok = require(ticks == 1, "timer stall was not coalesced") && ok;
    RF_HostPump();
    ok = require(ticks == 1, "timer caught up without deadline") && ok;
    RF_HostSetBootStage(5);
    SDL_zero(event); event.type = SDL_KEYDOWN; event.key.keysym.scancode = SDL_SCANCODE_W;
    SDL_PushEvent(&event);
    SDL_zero(event); event.type = SDL_MOUSEBUTTONDOWN; event.button.button = SDL_BUTTON_LEFT;
    SDL_PushEvent(&event); RF_HostPump();
    ok = require(!GetAsyncKeyState('W') && !GetAsyncKeyState(VK_LBUTTON), "stage-5 input was not zero") && ok;
    POINT pointer; RECT rect;
    ok = require(GetCursorPos(&pointer) && GetWindowRect(window, &rect) &&
        pointer.x == (rect.left+rect.right)/2 && pointer.y == (rect.top+rect.bottom)/2,
        "stage-5 neutral pointer was not window center") && ok;
    for (int frame = 1; frame < RF_BOOT_GAME_TICKS; ++frame) RF_HostBootFrame(5);
    RF_HostPump();
    ok = require(!RF_HostQuitRequested(), "stage-5 quit too early") && ok;
    RF_HostBootFrame(5); RF_HostPump();
    ok = require(RF_HostQuitRequested() && RF_HostBootFrameCount() == RF_BOOT_GAME_TICKS,
        "stage-5 final frame did not consume quit") && ok;
    RF_HostSetBootStage(0);
    SDL_zero(event); event.type = SDL_QUIT;
    SDL_PushEvent(&event); RF_HostPump();
    ok = require(RF_HostQuitRequested() != 0, "quit event lost") && ok;
    ok = require(!SetCursorPos(1, 1), "desktop cursor warp permitted") && ok;
    RF_HostDestroy(window);
    ok = require(SDL_WasInit(0) == 0, "SDL still initialized") && ok;
    fprintf(stderr, "host/input fixture: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
