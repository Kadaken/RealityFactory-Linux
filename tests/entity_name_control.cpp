/* Test-only wrapper: no avatar substitution or production startup bypass.
 * The ordinary actor-free level stops before the audio manager. Exercise that
 * real manager against the loaded world/registry after the expected -5 return. */
#include "RabidFramework.h"
#include <cstdio>
#include <cstdlib>
extern "C" int __real__ZN11CCommonData15InitializeLevelEPKc(CCommonData *, const char *);
extern "C" int __wrap__ZN11CCommonData15InitializeLevelEPKc(CCommonData *self, const char *level)
{
    const int result=__real__ZN11CCommonData15InitializeLevelEPKc(self,level);
    if (result!=-5 || !CCD->World() || !CCD->EntityRegistry()) {
        std::fprintf(stderr,"RF fixture-only: unexpected precondition in name control\n");
        std::abort();
    }
    {
        C3DAudioSource audio;
        const char *type=CCD->EntityRegistry()->GetEntityType("NeutralAudio");
        if (!type || strcmp(type,"AudioSource3D")) std::abort();
    }
    std::fprintf(stderr,"RF fixture-only: named audio constructed, registry matched, destructor returned\n");
    return result;
}
