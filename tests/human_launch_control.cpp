#include "host_window.h"
int RF_TestHumanHost(const char *profile);
// Exercise the actual native argument parser and human-mode gate, then replace
// inherited startup with the neutral host contract. All SDL show/capture calls
// remain intercepted by human_host_fixture.cpp.
extern "C" int __wrap__Z7WinMainPvS_Pci(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return RF_TestHumanHost("--human-launch");
}
