#include "rf_platform_compat.h"
#include "host_window.h"
#include "input_compat.h"

#include <SDL2/SDL.h>

static SDL_Scancode virtual_key_to_scancode(int key)
{
    if (key >= 'A' && key <= 'Z')
        return SDL_GetScancodeFromKey((SDL_Keycode)(key - 'A' + 'a'));
    if (key >= '0' && key <= '9')
        return SDL_GetScancodeFromKey((SDL_Keycode)key);
    if (key >= 0x60 && key <= 0x69)
        return key == 0x60 ? SDL_SCANCODE_KP_0 :
            (SDL_Scancode)(SDL_SCANCODE_KP_1 + key - 0x61);
    if (key >= VK_F1 && key <= VK_F12)
        return (SDL_Scancode)(SDL_SCANCODE_F1 + key - VK_F1);

    switch (key) {
    case VK_BACK: return SDL_SCANCODE_BACKSPACE;
    case VK_TAB: return SDL_SCANCODE_TAB;
    case VK_RETURN: return SDL_SCANCODE_RETURN;
    case VK_SHIFT: case VK_LSHIFT: return SDL_SCANCODE_LSHIFT;
    case VK_RSHIFT: return SDL_SCANCODE_RSHIFT;
    case VK_CONTROL: case VK_LCONTROL: return SDL_SCANCODE_LCTRL;
    case VK_RCONTROL: return SDL_SCANCODE_RCTRL;
    case VK_MENU: case VK_LMENU: return SDL_SCANCODE_LALT;
    case VK_RMENU: return SDL_SCANCODE_RALT;
    case VK_ESCAPE: return SDL_SCANCODE_ESCAPE;
    case VK_SPACE: return SDL_SCANCODE_SPACE;
    case VK_PRIOR: return SDL_SCANCODE_PAGEUP;
    case VK_NEXT: return SDL_SCANCODE_PAGEDOWN;
    case VK_END: return SDL_SCANCODE_END;
    case VK_HOME: return SDL_SCANCODE_HOME;
    case VK_LEFT: return SDL_SCANCODE_LEFT;
    case VK_UP: return SDL_SCANCODE_UP;
    case VK_RIGHT: return SDL_SCANCODE_RIGHT;
    case VK_DOWN: return SDL_SCANCODE_DOWN;
    case VK_SNAPSHOT: return SDL_SCANCODE_PRINTSCREEN;
    case VK_INSERT: return SDL_SCANCODE_INSERT;
    case VK_DELETE: return SDL_SCANCODE_DELETE;
    case VK_ADD: return SDL_SCANCODE_KP_PLUS;
    case VK_SUBTRACT: return SDL_SCANCODE_KP_MINUS;
    case VK_DECIMAL: return SDL_SCANCODE_KP_DECIMAL;
    case 0x6A: return SDL_SCANCODE_KP_MULTIPLY;
    case 0x6F: return SDL_SCANCODE_KP_DIVIDE;
    case 0xBA: return SDL_SCANCODE_SEMICOLON;
    case 0xBB: return SDL_SCANCODE_EQUALS;
    case 0xBC: return SDL_SCANCODE_COMMA;
    case 0xBD: return SDL_SCANCODE_MINUS;
    case 0xBE: return SDL_SCANCODE_PERIOD;
    case 0xBF: return SDL_SCANCODE_SLASH;
    case 0xC0: return SDL_SCANCODE_GRAVE;
    case 0xDB: return SDL_SCANCODE_LEFTBRACKET;
    case 0xDC: return SDL_SCANCODE_BACKSLASH;
    case 0xDD: return SDL_SCANCODE_RIGHTBRACKET;
    case 0xDE: return SDL_SCANCODE_APOSTROPHE;
    default: return SDL_SCANCODE_UNKNOWN;
    }
}

static BYTE key_state[256];
static bool held[SDL_NUM_SCANCODES];
static Uint32 mouse_buttons;
static Uint32 mouse_pressed_since_menu_frame;
static Uint32 mouse_released_since_menu_frame;
static Uint32 menu_synthetic_press;
static bool relative_motion;
static int motion_x, motion_y;
static int wheel_steps;

void RF_InputRelativeMode(bool enabled)
{
    relative_motion = enabled;
    motion_x = motion_y = 0;
    wheel_steps = 0;
}

void RF_InputTakeMotion(int *x, int *y)
{
    *x = motion_x; *y = motion_y;
    motion_x = motion_y = 0;
}

static int add_motion(int accumulated, int delta)
{
    // Bound queued hardware input before the inherited abs()/float conversion.
    const int64_t sum = (int64_t)accumulated + delta;
    return (int)(sum < -32767 ? -32767 : sum > 32767 ? 32767 : sum);
}

void RF_InputReset(void)
{
    memset(key_state, 0, sizeof(key_state));
    memset(held, 0, sizeof(held));
    mouse_buttons = 0;
    mouse_pressed_since_menu_frame = 0;
    mouse_released_since_menu_frame = 0;
    menu_synthetic_press = 0;
    motion_x = motion_y = 0;
    wheel_steps = 0;
}

void RF_InputEvent(const SDL_Event *event)
{
    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        RF_InputReset();
    } else if (event->type == SDL_MOUSEMOTION && relative_motion) {
        motion_x = add_motion(motion_x, event->motion.xrel);
        motion_y = add_motion(motion_y, event->motion.yrel);
    } else if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        const int scan = event->key.keysym.scancode;
        if (scan > SDL_SCANCODE_UNKNOWN && scan < SDL_NUM_SCANCODES)
            held[scan] = event->type == SDL_KEYDOWN;
    } else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
        if (event->button.button > 0 && event->button.button <= 32) {
            const Uint32 mask = 1u << (event->button.button - 1);
            if (event->type == SDL_MOUSEBUTTONDOWN) {
                mouse_buttons |= mask;
                mouse_pressed_since_menu_frame |= mask;
            } else {
                mouse_buttons &= ~mask;
                mouse_released_since_menu_frame |= mask;
            }
        }
    } else if (event->type == SDL_MOUSEWHEEL && event->wheel.y != 0) {
        int delta = event->wheel.y;
        if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) delta = -delta;
        const int64_t sum = (int64_t)wheel_steps + delta;
        wheel_steps = (int)(sum < -32 ? -32 : sum > 32 ? 32 : sum);
    }
    for (int key = 0; key < 256; ++key) {
        const SDL_Scancode scan = virtual_key_to_scancode(key);
        key_state[key] = scan != SDL_SCANCODE_UNKNOWN && held[scan] ? 0x80 : 0;
    }
    key_state[VK_SHIFT] = key_state[VK_LSHIFT] | key_state[VK_RSHIFT];
    key_state[VK_CONTROL] = key_state[VK_LCONTROL] | key_state[VK_RCONTROL];
    key_state[VK_MENU] = key_state[VK_LMENU] | key_state[VK_RMENU];
    key_state[VK_LBUTTON] = mouse_buttons & SDL_BUTTON_LMASK ? 0x80 : 0;
    key_state[VK_RBUTTON] = mouse_buttons & SDL_BUTTON_RMASK ? 0x80 : 0;
    key_state[VK_MBUTTON] = mouse_buttons & SDL_BUTTON_MMASK ? 0x80 : 0;
}

int RF_InputTakeWheelStep(void)
{
    if (wheel_steps > 0) { --wheel_steps; return 1; }
    if (wheel_steps < 0) { ++wheel_steps; return -1; }
    return 0;
}

void RF_InputMenuFrame(Uint32 physical_buttons)
{
    const Uint32 supported = SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK;
    physical_buttons &= supported;
    if (menu_synthetic_press) {
        /* A complete click arrived between menu frames. Present its release
           now so the inherited press-then-release action contract completes. */
        menu_synthetic_press = 0;
        mouse_buttons = physical_buttons;
    } else {
        const Uint32 complete_click = mouse_pressed_since_menu_frame &
            mouse_released_since_menu_frame & ~physical_buttons;
        menu_synthetic_press = complete_click;
        mouse_buttons = physical_buttons | complete_click;
    }
    mouse_pressed_since_menu_frame = 0;
    mouse_released_since_menu_frame = 0;
    key_state[VK_LBUTTON] = mouse_buttons & SDL_BUTTON_LMASK ? 0x80 : 0;
    key_state[VK_RBUTTON] = mouse_buttons & SDL_BUTTON_RMASK ? 0x80 : 0;
    key_state[VK_MBUTTON] = mouse_buttons & SDL_BUTTON_MMASK ? 0x80 : 0;
}

extern "C" SHORT GetAsyncKeyState(int key)
{
    // Legacy release-wait loops call only this API. Route through the one host
    // pump so a key-up can arrive even without a surrounding PeekMessage loop.
    RF_HostPump();
    return key >= 0 && key < 256 && (key_state[key] & 0x80) ? (SHORT)0x8000 : 0;
}
extern "C" BOOL GetKeyboardState(BYTE *state)
{
    RF_HostPump();
    if (!state) return FALSE;
    memcpy(state, key_state, sizeof(key_state));
    return TRUE;
}
extern "C" int ToAscii(UINT key, UINT, const BYTE *state, WORD *characters, UINT)
{
    if (!state || !characters) return 0;
    const bool shift = (state[VK_SHIFT] & 0x80) != 0;
    // This legacy API returns ASCII only. SDL text/IME composition is not emulated.
    if (key >= 'A' && key <= 'Z') {
        const bool caps = (SDL_GetModState() & KMOD_CAPS) != 0;
        *characters = (WORD)((shift != caps) ? key : key + ('a'-'A'));
    } else if (key >= '0' && key <= '9') {
        *characters = shift ? ")!@#$%^&*("[key-'0'] : (WORD)key;
    } else if (key == VK_SPACE || key == VK_RETURN || key == VK_TAB || key == VK_BACK) {
        *characters = (WORD)key;
    } else if (key >= 0x60 && key <= 0x69) {
        *characters = (WORD)('0' + key - 0x60);
    } else {
        const UINT codes[] = {0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xDB,0xDC,0xDD,0xDE,0x6A,0x6B,0x6D,0x6E,0x6F};
        const char plain[] = ";=,-./`[\\]'*+-./";
        const char shifted[] = ":+<_>?~{|}\"*+-./";
        for (size_t i=0; i<sizeof(codes)/sizeof(codes[0]); ++i) {
            if (key == codes[i]) {
                *characters = (WORD)(shift ? shifted[i] : plain[i]);
                return 1;
            }
        }
        return 0;
    }
    return 1;
}
extern "C" int GetSystemMetrics(int) { return 0; }
extern "C" BOOL PeekMessage(MSG *message, HWND, UINT, UINT, UINT remove)
{
    return RF_HostMessage(message, remove != PM_NOREMOVE);
}
extern "C" BOOL GetMessage(MSG *message, HWND, UINT, UINT)
{
    return RF_HostMessage(message, 1);
}
extern "C" BOOL TranslateMessage(const MSG *) { return TRUE; }
extern "C" LRESULT DispatchMessage(const MSG *) { return 0; } // Host already dispatches.

extern "C" BOOL GetClientRect(HWND window, RECT *rect)
{
    SDL_Window *sdl_window = (SDL_Window *)window;
    int width;
    int height;
    if (rect == NULL)
        return FALSE;
    if (sdl_window == NULL)
        sdl_window = SDL_GetKeyboardFocus();
    if (sdl_window == NULL)
        return FALSE;
    SDL_GetWindowSize(sdl_window, &width, &height);
    rect->left = rect->top = 0;
    rect->right = width;
    rect->bottom = height;
    return TRUE;
}

extern "C" BOOL ClientToScreen(HWND window, POINT *point)
{
    SDL_Window *sdl_window = (SDL_Window *)window;
    int x;
    int y;
    if (point == NULL)
        return FALSE;
    if (sdl_window == NULL)
        sdl_window = SDL_GetKeyboardFocus();
    if (sdl_window == NULL)
        return FALSE;
    SDL_GetWindowPosition(sdl_window, &x, &y);
    point->x += x;
    point->y += y;
    return TRUE;
}

extern "C" BOOL GetCursorPos(POINT *point)
{
    int x;
    int y;
    if (point == NULL)
        return FALSE;
    if (RF_HostNeutralPointer(point)) return TRUE;
    if (RF_HostRelativePointer(point)) return TRUE;
    SDL_GetGlobalMouseState(&x, &y);
    point->x = x;
    point->y = y;
    return TRUE;
}

extern "C" BOOL SetCursorPos(int x, int y)
{
    // Never warp the physical desktop cursor from legacy polling loops.
    (void)x; (void)y;
    return FALSE;
}
