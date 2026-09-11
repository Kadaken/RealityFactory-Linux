/* Native host boundary. SDL window belongs to RF, GL context to the driver. */
#include "host_window.h"
#include "input_compat.h"
#include "Genesis.h"
#include <dlfcn.h>
#include <deque>
#include <string>

static SDL_Window *window;
static WNDPROC callback;
static bool human_launch, quit_requested;
static bool gameplay, menu_active, escape_released_capture, relative_enabled, focus_lost;
static bool requested_cursor_visible = true;
static bool neutral_boot;
static int boot_stage, boot_frames;
static bool boot_failed;
static std::deque<MSG> messages;
static std::string driver_directory;
static Uint64 timer_deadline;
static UINT timer_interval;
static geEngine *fatal_engine;
static geSound_System *fatal_audio;

static void sync_cursor_visibility()
{
    if (!human_launch) return;
    const bool outside = !window || focus_lost || quit_requested ||
        (window && SDL_GetMouseFocus() != window);
    const bool show = outside || (!menu_active && !relative_enabled && requested_cursor_visible);
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

static void sync_relative_mode()
{
    const bool wanted = window && human_launch && !neutral_boot && !boot_stage &&
        gameplay && !quit_requested && !escape_released_capture && !focus_lost &&
        SDL_GetKeyboardFocus() == window;
    if (wanted == relative_enabled) return;
    if (SDL_SetRelativeMouseMode(wanted ? SDL_TRUE : SDL_FALSE) != 0) {
        fprintf(stderr, "RF host: relative mouse %s failed: %s; leave gameplay and inspect SDL input\n",
            wanted ? "enable" : "release", SDL_GetError());
        // An enable failure must never feed absolute cursor motion into look.
        escape_released_capture = true;
        return;
    }
    relative_enabled = wanted;
    RF_InputRelativeMode(wanted);
    sync_cursor_visibility();
    fprintf(stderr, "RF host: relative mouse %s\n", wanted ? "on" : "off");
}

extern "C" int RF_HostSetMenuActive(int active)
{
    const int previous = menu_active;
    menu_active = active != 0;
    sync_cursor_visibility();
    return previous;
}

extern "C" int RF_HostMenuPointer(POINT *point, int logical_width, int logical_height)
{
    int x, y, window_width, window_height;
    if (!window || !point || logical_width <= 0 || logical_height <= 0)
        return 0;
    const Uint32 buttons = SDL_GetMouseState(&x, &y);
    SDL_GetWindowSize(window, &window_width, &window_height);
    if (window_width <= 0 || window_height <= 0)
        return 0;
    RF_InputMenuFrame(buttons);
    point->x = (long)(((int64_t)x * logical_width) / window_width);
    point->y = (long)(((int64_t)y * logical_height) / window_height);
    if (getenv("RF_DIAG_MENU_POINTER")) {
        int global_x, global_y, window_x, window_y;
        const Uint32 global_buttons = SDL_GetGlobalMouseState(&global_x, &global_y);
        SDL_GetWindowPosition(window, &window_x, &window_y);
        fprintf(stderr,
            "RF D25: local=%d,%d local_buttons=%u global=%d,%d global_buttons=%u origin=%d,%d hit=%d,%d logical=%d,%d window=%d,%d\n",
            x, y, (unsigned)buttons, global_x, global_y, (unsigned)global_buttons,
            window_x, window_y, point->x, point->y, logical_width, logical_height,
            window_width, window_height);
    }
    return 1;
}

extern "C" int RF_HostSetGameplay(int active)
{
    const int previous = gameplay;
    gameplay = active != 0;
    if (!gameplay || previous != (int)gameplay) escape_released_capture = false;
    sync_relative_mode();
    return previous;
}

extern "C" int RF_HostRelativePointer(POINT *point)
{
    if (!window || !human_launch || !gameplay || !point) return 0;
    // The inherited look code subtracts screen-space client centre. Supply
    // centre + consumed SDL deltas, or centre when capture is unavailable.
    int x, y, width, height, dx = 0, dy = 0;
    SDL_GetWindowPosition(window, &x, &y);
    SDL_GetWindowSize(window, &width, &height);
    if (relative_enabled) RF_InputTakeMotion(&dx, &dy);
    point->x = x + width/2 + dx;
    point->y = y + height/2 + dy;
    return 1;
}

static int pointer_display()
{
    int x, y;
    SDL_GetGlobalMouseState(&x, &y);
    const int count = SDL_GetNumVideoDisplays();
    for (int i = 0; i < count; ++i) {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(i, &bounds) != 0) {
            fprintf(stderr, "RF host: display bounds unavailable: %s\n", SDL_GetError());
            continue;
        }
        if ((int64_t)x >= bounds.x && (int64_t)y >= bounds.y &&
            (int64_t)x < (int64_t)bounds.x + bounds.w &&
            (int64_t)y < (int64_t)bounds.y + bounds.h) return i;
    }
    fprintf(stderr, "RF host: pointer display unavailable; using display 0\n");
    return 0;
}
extern "C" void RF_HostBindEngine(void *engine) { fatal_engine = (geEngine *)engine; }
extern "C" void RF_HostBindAudio(void *audio) { fatal_audio = (geSound_System *)audio; }
extern "C" int RF_HostFatalShutdown(void)
{
    gameplay = false;
    sync_relative_mode();
    if (fatal_audio) {
        geSound_System *audio = fatal_audio; fatal_audio = NULL;
        geSound_DestroySoundSystem(audio);
    }
    if (fatal_engine) {
        geEngine *engine = fatal_engine; fatal_engine = NULL;
        geEngine_Free(engine); // Driver shutdown owns GL context deletion.
    }
    if (SDL_GL_GetCurrentContext()) {
        fprintf(stderr, "RF fatal: live GL context after engine teardown\n");
        return 0;
    }
    RF_HostDestroy(window);
    fprintf(stderr, "RF fatal: host teardown complete\n");
    return 1;
}

extern "C" void RF_HostConfigure(int human) { human_launch = human != 0; }
extern "C" int RF_HostHumanLaunch(void) { return human_launch; }
extern "C" void RF_HostSetNeutralBoot(int enabled) { neutral_boot = enabled != 0; }
extern "C" int RF_HostNeutralBoot(void) { return neutral_boot; }
extern "C" void RF_HostSetBootStage(int stage) { boot_stage = stage; boot_frames = 0; boot_failed = false; }
extern "C" int RF_HostBootStage(void) { return boot_stage; }
extern "C" int RF_HostBootFrameCount(void) { return boot_failed ? -1 : boot_frames; }
extern "C" void RF_HostConfigDiagnostic(const char *filename)
{
    char cwd[PATH_MAX];
    fprintf(stderr, "RF config: requested %s; resolved config dir: %s/install (filesystem fallback)\n",
            filename, getcwd(cwd, sizeof(cwd)) ? cwd : "<getcwd failed>");
}
extern "C" void RF_HostBootFrame(int stage)
{
    if (boot_stage != stage) return;
    if (!window || !(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN)) boot_failed = true;
    ++boot_frames;
    if (stage == 5 && boot_frames < RF_BOOT_GAME_TICKS) return;
    SDL_Event quit = {};
    quit.type = SDL_QUIT;
    if (SDL_PushEvent(&quit) != 1) {
        fprintf(stderr, "RF boot: SDL quit notification failed: %s\n", SDL_GetError());
        boot_failed = true;
        quit_requested = true;
    }
}
extern "C" int RF_HostQuitRequested(void) { return quit_requested; }

extern "C" int RF_HostNeutralPointer(POINT *point)
{
    if (boot_stage != 5 || !window || !point) return 0;
    int x, y, width, height;
    SDL_GetWindowPosition(window, &x, &y);
    SDL_GetWindowSize(window, &width, &height);
    point->x = x + width/2;
    point->y = y + height/2;
    return 1;
}

extern "C" const char *RF_HostDriverDirectory(void)
{
    Dl_info info;
    char canonical[PATH_MAX];
    // Taking an imported function address can resolve to the executable's PLT.
    // dlsym obtains the actual implementation address in the loaded DSO.
    void *engine_symbol = dlsym(RTLD_DEFAULT, "geEngine_CreateWithVersion");
    if (!engine_symbol || !dladdr(engine_symbol, &info) || !info.dli_fname ||
        !realpath(info.dli_fname, canonical)) {
        fprintf(stderr, "RF host: cannot locate loaded engine library; check runtime library search paths\n");
        return NULL;
    }
    driver_directory = canonical;
    driver_directory.erase(driver_directory.find_last_of('/'));
    const std::string module = driver_directory + "/libOglDrv.so";
    if (access(module.c_str(), R_OK) != 0) {
        fprintf(stderr, "RF host: missing readable driver %s; build the sibling OglDrv target\n", module.c_str());
        return NULL;
    }
    return driver_directory.c_str();
}

extern "C" HWND RF_HostCreate(const char *title, int width, int height, WNDPROC proc)
{
    if (window || width <= 0 || height <= 0) {
        fprintf(stderr, "RF host: one window supported; dimensions must be positive\n");
        return NULL;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "RF host: SDL initialization failed: %s\n", SDL_GetError());
        return NULL;
    }
    fprintf(stderr, "RF host: SDL video driver %s\n",
        SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<unknown>");
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8) != 0) {
        fprintf(stderr, "RF host: GL attributes failed: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }
    int position = SDL_WINDOWPOS_UNDEFINED;
    if (human_launch) {
        const int display = pointer_display();
        position = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
        fprintf(stderr, "RF host: window on display %d\n", display);
    }
    window = SDL_CreateWindow(title, position, position,
                              width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "RF host: window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }
    callback = proc;
    quit_requested = false;
    gameplay = menu_active = escape_released_capture = relative_enabled = focus_lost = false;
    requested_cursor_visible = true;
    RF_InputRelativeMode(false);
    RF_InputReset();
    return window;
}

extern "C" void RF_HostDestroy(HWND owned)
{
    if (!owned || owned != window) return;
    gameplay = false;
    sync_relative_mode();
    if (SDL_GL_GetCurrentContext()) {
        fprintf(stderr, "RF host: refusing window teardown before geEngine_Free\n");
        return;
    }
    timer_deadline = timer_interval = 0;
    callback = NULL;
    messages.clear();
    RF_InputReset();
    SDL_DestroyWindow(window);
    window = NULL;
    sync_cursor_visibility();
    SDL_Quit();
}

extern "C" void RF_HostPump(void)
{
    static bool pumping;
    if (!window || pumping) return;
    struct PumpGuard {
        bool &active;
        PumpGuard(bool &value) : active(value) { active = true; }
        ~PumpGuard() { active = false; }
    } guard(pumping);
    SDL_Event event;
    sync_relative_mode();
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            focus_lost = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            focus_lost = false;
        if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            escape_released_capture = true;
        if (boot_stage == 5) RF_InputReset();
        else RF_InputEvent(&event);
        MSG message;
        memset(&message, 0, sizeof(message));
        message.hwnd = window;
        if (event.type == SDL_QUIT ||
            (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
            quit_requested = true;
            message.message = 0x0012; // WM_QUIT
        } else if (event.type == SDL_WINDOWEVENT) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED: if (boot_stage != 5) message.message = WM_SETFOCUS; break;
            case SDL_WINDOWEVENT_FOCUS_LOST: if (boot_stage != 5) message.message = WM_KILLFOCUS; break;
            case SDL_WINDOWEVENT_SIZE_CHANGED: message.message = WM_WINDOWPOSCHANGED; break;
            default: break;
            }
        }
        if (message.message) {
            if (callback) callback(window, message.message, 0, 0);
            // Bounded legacy notification queue; input state is kept separately.
            if (messages.size() == 256) messages.pop_front();
            messages.push_back(message);
        }
        sync_relative_mode();
        sync_cursor_visibility();
    }
    const Uint64 now = SDL_GetTicks64();
    if (timer_interval && now >= timer_deadline) {
        timer_deadline = now + timer_interval; // Coalesce stalls; never catch up in a loop.
        if (callback) callback(window, WM_TIMER, 1, 0);
    }
}

extern "C" BOOL RF_HostMessage(MSG *message, int remove)
{
    RF_HostPump();
    if (!message || messages.empty()) return FALSE;
    *message = messages.front();
    if (remove) messages.pop_front();
    return TRUE;
}

extern "C" BOOL GetWindowRect(HWND owned, RECT *rect)
{
    if (!rect) return FALSE;
    int x, y, w, h;
    if (!owned) {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(0, &bounds) != 0) return FALSE;
        x = bounds.x; y = bounds.y; w = bounds.w; h = bounds.h;
    } else {
        SDL_GetWindowPosition((SDL_Window *)owned, &x, &y);
        SDL_GetWindowSize((SDL_Window *)owned, &w, &h);
    }
    rect->left = x; rect->top = y; rect->right = x+w; rect->bottom = y+h;
    return TRUE;
}
extern "C" HWND GetDesktopWindow(void) { return NULL; }
extern "C" BOOL SetWindowPos(HWND owned, HWND, int x, int y, int w, int h, UINT flags)
{
    if (!owned) return FALSE;
    SDL_SetWindowPosition((SDL_Window *)owned, x, y);
    if (!(flags & SWP_NOSIZE)) SDL_SetWindowSize((SDL_Window *)owned, w, h);
    if (flags & SWP_SHOWWINDOW) ShowWindow(owned, SW_SHOWNORMAL);
    return TRUE;
}
extern "C" BOOL ShowWindow(HWND owned, int command)
{
    if (!owned) return FALSE;
    if (!human_launch) return FALSE;
    if (command == SW_MINIMIZE) SDL_MinimizeWindow((SDL_Window *)owned);
    else SDL_ShowWindow((SDL_Window *)owned);
    return TRUE;
}
extern "C" BOOL UpdateWindow(HWND owned) { return owned != NULL; }
extern "C" BOOL GetOpenFileName(OPENFILENAME *)
{
    fprintf(stderr, "RF host: native file dialog unavailable; supply a map on the command line\n");
    return FALSE;
}
extern "C" int MessageBox(HWND, const char *text, const char *title, UINT)
{
    fprintf(stderr, "RF dialog [%s]: %s\n", title ? title : "", text ? text : "");
    return IDOK;
}
extern "C" int ShowCursor(BOOL show)
{
    if (!human_launch) return 0;
    requested_cursor_visible = show != FALSE;
    sync_cursor_visibility();
    return requested_cursor_visible ? 1 : 0;
}
extern "C" UINT_PTR SetTimer(HWND owned, UINT_PTR id, UINT interval, void *proc)
{
    if (owned != window || !window || proc || (id != 0 && id != 1) || !interval) {
        fprintf(stderr, "RF host: unsupported timer contract; requires main window, null callback and positive interval\n");
        return 0;
    }
    timer_interval = interval;
    timer_deadline = SDL_GetTicks64() + interval;
    return 1;
}
