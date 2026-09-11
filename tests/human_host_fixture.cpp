// Real hidden SDL/X11 window, synthetic displays/focus/relative-mode backend.
// No test may show a window or enable real pointer capture, even in human mode.
#include "host_window.h"
#include "input_compat.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <cstdio>
#include <climits>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "human host: failed line %d\n", __LINE__); return 1; } } while (0)
static SDL_Window *owned, *focus;
static bool mock_displays, reject_capture;
static int pointer_x, pointer_y, local_x, local_y, position_x, position_y, enabled, disabled;
static Uint32 local_buttons;
static SDL_Window *mouse_focus;
static int cursor_state = SDL_ENABLE;
static bool forbid_global_mouse;
static bool forbidden_global_mouse_called;
static SDL_bool mode;
extern "C" SDL_Window *__real_SDL_CreateWindow(const char *, int, int, int, int, Uint32);
extern "C" SDL_Window *__wrap_SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags)
{
    position_x = x; position_y = y;
    if (!(flags & SDL_WINDOW_HIDDEN)) return NULL;
    owned = __real_SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, w, h, flags);
    return owned;
}
extern "C" void __wrap_SDL_ShowWindow(SDL_Window *) { fprintf(stderr, "human host: show intercepted; remains hidden\n"); }
extern "C" SDL_Window *__wrap_SDL_GetKeyboardFocus(void) { return focus; }
extern "C" int __wrap_SDL_SetRelativeMouseMode(SDL_bool value)
{
    if (reject_capture && value) return SDL_SetError("fixture capture rejection");
    mode = value;
    if (value) ++enabled; else ++disabled;
    return 0;
}
extern "C" int __real_SDL_GetNumVideoDisplays(void);
extern "C" int __wrap_SDL_GetNumVideoDisplays(void)
{ return mock_displays ? 2 : __real_SDL_GetNumVideoDisplays(); }
extern "C" int __real_SDL_GetDisplayBounds(int, SDL_Rect *);
extern "C" int __wrap_SDL_GetDisplayBounds(int index, SDL_Rect *r)
{
    if (!mock_displays) return __real_SDL_GetDisplayBounds(index, r);
    r->x = index ? 0 : -1920; r->y = index ? 1080 : 0;
    r->w = 1920; r->h = 1080; return 0;
}
extern "C" Uint32 __wrap_SDL_GetGlobalMouseState(int *x, int *y)
{
    if (forbid_global_mouse) forbidden_global_mouse_called = true;
    *x = pointer_x; *y = pointer_y; return 0;
}
extern "C" Uint32 __wrap_SDL_GetMouseState(int *x, int *y)
{ *x = local_x; *y = local_y; return local_buttons; }
extern "C" SDL_Window *__wrap_SDL_GetMouseFocus(void) { return mouse_focus; }
extern "C" int __wrap_SDL_ShowCursor(int toggle)
{
    if (toggle != SDL_QUERY) cursor_state = toggle;
    return cursor_state;
}

static void window_event(Uint8 type)
{
    SDL_Event e; SDL_zero(e);
    e.type = SDL_WINDOWEVENT; e.window.windowID = SDL_GetWindowID(owned);
    e.window.event = type; SDL_PushEvent(&e); RF_HostPump();
}
static void key(SDL_Scancode scan)
{
    SDL_Event e; SDL_zero(e); e.type = SDL_KEYDOWN;
    e.key.windowID = SDL_GetWindowID(owned); e.key.keysym.scancode = scan;
    SDL_PushEvent(&e); RF_HostPump();
}
static void motion(int x, int y)
{
    SDL_Event e; SDL_zero(e); e.type = SDL_MOUSEMOTION;
    e.motion.windowID = SDL_GetWindowID(owned); e.motion.xrel = x; e.motion.yrel = y;
    SDL_PushEvent(&e); RF_HostPump();
}
static void button(Uint32 type, Uint8 value)
{
    SDL_Event e; SDL_zero(e); e.type = type;
    e.button.windowID = SDL_GetWindowID(owned); e.button.button = value;
    SDL_PushEvent(&e);
}
static bool create(bool human, int x, int y)
{
    RF_HostConfigure(human);
    pointer_x = x; pointer_y = y;
    return RF_HostCreate("Neutral human host dry run", 320, 240, NULL) != NULL;
}
static void destroy()
{
    focus = mouse_focus = NULL;
    RF_HostDestroy(owned); owned = NULL;
}
int RF_TestHumanHost(const char *profile)
{
    if (!strcmp(profile, "placement")) {
        mock_displays = true;
        const int points[][3] = {{-1, 0, 0}, {0, 1080, 1}, {1919, 2159, 1},
            {1920, 2159, 0}, {0, 1079, 0}, {-1921, 0, 0}};
        for (unsigned i = 0; i < sizeof(points)/sizeof(points[0]); ++i) {
            CHECK(create(true, points[i][0], points[i][1]));
            CHECK(position_x == (int)SDL_WINDOWPOS_CENTERED_DISPLAY(points[i][2]));
            CHECK(position_y == position_x); destroy();
        }
        mock_displays = false;
        CHECK(create(true, INT_MAX, INT_MAX));
        CHECK(position_x == (int)SDL_WINDOWPOS_CENTERED_DISPLAY(0)); destroy();
    } else if (!strcmp(profile, "hidden")) {
        for (int stage = 0; stage <= 5; ++stage) {
            RF_HostSetBootStage(stage);
            CHECK(create(false, 0, 0)); focus = owned;
            CHECK(position_x == (int)SDL_WINDOWPOS_UNDEFINED);
            RF_HostSetGameplay(1); window_event(SDL_WINDOWEVENT_FOCUS_GAINED);
            CHECK(!mode && !SDL_GetRelativeMouseMode() && enabled == 0);
            CHECK(!ShowWindow(owned, SW_SHOWNORMAL)); destroy();
        }
    } else if (!strcmp(profile, "pointer")) {
        CHECK(create(true, 50, 50));
        focus = mouse_focus = owned;
        window_event(SDL_WINDOWEVENT_FOCUS_GAINED);
        SDL_SetWindowSize(owned, 320, 240);
        SDL_SetWindowPosition(owned, 137, 211);
        local_x = 160; local_y = 120; local_buttons = 0;
        {
            RF_HostMenuScope menu;
            POINT p;
            CHECK(cursor_state == SDL_DISABLE);
            forbid_global_mouse = true;
            CHECK(RF_HostMenuPointer(&p, 1024, 768));
            CHECK(!forbidden_global_mouse_called);
            CHECK(p.x == 512 && p.y == 384);
            SDL_SetWindowPosition(owned, 301, 47);
            CHECK(RF_HostMenuPointer(&p, 1024, 768));
            CHECK(p.x == 512 && p.y == 384);
            SDL_SetWindowSize(owned, 640, 480);
            local_x = 320; local_y = 240;
            CHECK(RF_HostMenuPointer(&p, 1024, 768));
            CHECK(p.x == 512 && p.y == 384);
            button(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT);
            button(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
            RF_HostPump();
            CHECK(RF_HostMenuPointer(&p, 1024, 768));
            CHECK((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
            CHECK(RF_HostMenuPointer(&p, 1024, 768));
            CHECK((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0);
            window_event(SDL_WINDOWEVENT_FOCUS_LOST);
            CHECK(cursor_state == SDL_ENABLE);
            mouse_focus = owned;
            window_event(SDL_WINDOWEVENT_FOCUS_GAINED);
            CHECK(cursor_state == SDL_DISABLE);
            forbid_global_mouse = false;
        }
        CHECK(cursor_state == SDL_ENABLE);
        destroy(); CHECK(cursor_state == SDL_ENABLE);
    } else if (!strcmp(profile, "--human-launch") || !strcmp(profile, "menu")) {
        if (!strcmp(profile, "--human-launch")) CHECK(RF_HostHumanLaunch());
        CHECK(create(true, 0, 0));
        CHECK(ShowWindow(owned, SW_SHOWNORMAL));
        CHECK(SDL_GetWindowFlags(owned) & SDL_WINDOW_HIDDEN);
        CHECK(!mode); focus = mouse_focus = owned;
        RF_HostSetGameplay(1); CHECK(mode && enabled == 1);
        RECT rect; POINT p; GetWindowRect(owned, &rect);
        const int cx = (rect.left + rect.right)/2, cy = (rect.top + rect.bottom)/2;
        motion(7, -3); motion(2, 1);
        CHECK(GetCursorPos(&p) && p.x == cx+9 && p.y == cy-2);
        CHECK(GetCursorPos(&p) && p.x == cx && p.y == cy); // Consume exactly once.
        // Exercise adapter saturation independently of SDL's processing of
        // out-of-range synthetic hardware input.
        SDL_Event huge; SDL_zero(huge); huge.type = SDL_MOUSEMOTION;
        huge.motion.xrel = INT_MAX; huge.motion.yrel = INT_MIN;
        RF_InputEvent(&huge);
        CHECK(GetCursorPos(&p) && p.x == cx+32767 && p.y == cy-32767);
        CHECK(!SetCursorPos(1, 1));
        // Even a stale focus getter must not override an explicit loss event.
        motion(5, 5); window_event(SDL_WINDOWEVENT_FOCUS_LOST); CHECK(!mode);
        CHECK(GetCursorPos(&p) && p.x == cx && p.y == cy);
        focus = NULL; RF_HostPump(); CHECK(!mode);
        focus = mouse_focus = owned; window_event(SDL_WINDOWEVENT_FOCUS_GAINED); CHECK(mode);
        { RF_HostGameplayScope menu(0); CHECK(!mode);
          motion(12, 12); RF_HostPump(); CHECK(!mode); }
        CHECK(mode); CHECK(GetCursorPos(&p) && p.x == cx && p.y == cy);
        key(SDL_SCANCODE_ESCAPE); CHECK(!mode); RF_HostPump(); CHECK(!mode);
        RF_HostSetGameplay(0); RF_HostSetGameplay(1); CHECK(mode);
        RF_HostSetGameplay(0); reject_capture = true;
        RF_HostSetGameplay(1); CHECK(!mode);
        CHECK(GetCursorPos(&p) && p.x == cx && p.y == cy);
        reject_capture = false; RF_HostSetGameplay(0); RF_HostSetGameplay(1); CHECK(mode);
        SDL_Event e; SDL_zero(e); e.type = SDL_QUIT; SDL_PushEvent(&e);
        RF_HostPump(); CHECK(RF_HostQuitRequested() && !mode);
        RF_HostSetGameplay(1); CHECK(!mode);
        CHECK(!SDL_GetRelativeMouseMode()); // Real SDL capture was never enabled.
        destroy(); CHECK(!mode && disabled == enabled);
    } else return 2;
    CHECK(SDL_WasInit(0) == 0);
    fprintf(stderr, "human host: PASS %s (hidden, capture backend mocked)\n", profile);
    return 0;
}
