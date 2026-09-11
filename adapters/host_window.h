#ifndef RF_HOST_WINDOW_H
#define RF_HOST_WINDOW_H
#include "rf_platform_compat.h"
#ifdef __cplusplus
extern "C" {
#endif
void RF_HostConfigure(int human_launch);
void RF_HostSetNeutralBoot(int enabled);
int RF_HostNeutralBoot(void);
void RF_HostSetBootStage(int stage);
int RF_HostBootStage(void);
void RF_HostBindEngine(void *engine);
void RF_HostBindAudio(void *audio);
int RF_HostFatalShutdown(void);
void RF_HostConfigDiagnostic(const char *filename);
void RF_HostBootFrame(int stage);
int RF_HostBootFrameCount(void);
enum { RF_BOOT_GAME_TICKS = 120 };
int RF_HostNeutralPointer(POINT *point);
int RF_RunBootStage(void);
int RF_RunNeutralFrame(void);
HWND RF_HostCreate(const char *title, int width, int height, WNDPROC callback);
void RF_HostDestroy(HWND window);
const char *RF_HostDriverDirectory(void);
int RF_HostHumanLaunch(void);
void RF_HostPump(void);
int RF_HostQuitRequested(void);
int RF_HostSetGameplay(int active);
int RF_HostRelativePointer(POINT *point);
int RF_HostMenuPointer(POINT *point, int logical_width, int logical_height);
int RF_HostSetMenuActive(int active);
BOOL RF_HostMessage(MSG *message, int remove);
#ifdef __cplusplus
}
// InGame means a level exists, even while paused. Scope the actual execution
// path instead; all returns (including level changes) restore the prior state.
class RF_HostGameplayScope {
    int previous;
    RF_HostGameplayScope(const RF_HostGameplayScope &);
    RF_HostGameplayScope &operator=(const RF_HostGameplayScope &);
public:
    explicit RF_HostGameplayScope(int active) : previous(RF_HostSetGameplay(active)) {}
    ~RF_HostGameplayScope() { RF_HostSetGameplay(previous); }
};
class RF_HostMenuScope {
    int previous;
    RF_HostMenuScope(const RF_HostMenuScope &);
    RF_HostMenuScope &operator=(const RF_HostMenuScope &);
public:
    RF_HostMenuScope() : previous(RF_HostSetMenuActive(1)) {}
    ~RF_HostMenuScope() { RF_HostSetMenuActive(previous); }
};
#endif
#endif
