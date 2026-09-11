/* Only the neutral Inventory section is exercised; no actor/player is faked. */
#include "RabidFramework.h"
#include <cstdio>
#include <cstdlib>

extern "C" geBoolean __real_geEngine_DrawBitmap(const geEngine *,const geBitmap *,const geRect *,uint32,uint32);
extern "C" geBoolean __wrap_geEngine_DrawBitmap(const geEngine *engine,const geBitmap *bitmap,const geRect *rect,uint32 x,uint32 y)
{
    if (!bitmap) {
        std::fprintf(stderr,"RF inventory fixture: null bitmap draw rejected\n");
        std::abort();
    }
    return __real_geEngine_DrawBitmap(engine,bitmap,rect,x,y);
}

extern "C" int __wrap_RF_RunBootStage(void)
{
    {
        CInventory inventory;
        std::fprintf(stderr,"RF inventory fixture: constructor returned\n");
        // Test-only access checking is disabled for this translation unit.
        // Set paging state without inventing a player or changing class layout;
        // the real Blit runs with empty item slots and generated menu images.
        inventory.MaxItems=10;
        inventory.MaxWeapons=10;
        for(int menu=WEAPONINV;menu<=ITEMINV;++menu) {
            for(int selection=-1;selection<=9;selection+=10) {
                inventory.Selected=selection;
                if(CCD->Engine()->BeginFrame()!=RGF_SUCCESS) return 1;
                inventory.Blit(menu);
                if(CCD->Engine()->EndFrame()!=RGF_SUCCESS) return 1;
            }
        }
        std::fprintf(stderr,"RF inventory fixture: four paginated draw branches returned\n");
    }
    std::fprintf(stderr,"RF inventory fixture: destructor returned\n");
    return 0;
}
