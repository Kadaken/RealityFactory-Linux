/* Test-only boot-stage wrapper for D21: CWeapon::SetWeapon must not index
 * WeaponD with the -1 "no current weapon" sentinel. Linked against the
 * unchanged runtime objects at neutral boot stage 2 (common init, no level).
 *
 * Three link wraps keep this free of level data:
 *   RF_RunBootStage        - replaces the stage body with this control
 *   geWorld_GetEntitySet   - returns NULL so the CWeapon constructor takes its
 *                            inherited "no PlayerSetup" early return
 *   CActorManager::GetMotion - returns a non-empty motion so the old-animation
 *                            scan is genuinely reached
 * The translation unit is compiled with -fno-access-control only to install
 * the player pointer that SetWeapon dereferences.
 */
#include "RabidFramework.h"
#include <cstdio>
#include <cstring>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "weapon sentinel: FAIL line %d\n", __LINE__); return 1; } } while (0)

extern "C" geEntity_EntitySet *__wrap_geWorld_GetEntitySet(geWorld *, const char *)
{
    return NULL;
}

extern "C" char *_ZN13CActorManager9GetMotionEPK7geActor(CActorManager *, const geActor *);
extern "C" char *__wrap__ZN13CActorManager9GetMotionEPK7geActor(CActorManager *, const geActor *)
{
    static char idle[] = "Idle";
    return idle;
}

static int motion_sets;
static char selected_motion[64];
extern "C" int __wrap__ZN13CActorManager9SetMotionEPK7geActorPKci(
    CActorManager *, const geActor *, const char *motion, int)
{
    ++motion_sets;
    std::snprintf(selected_motion, sizeof(selected_motion), "%s", motion ? motion : "<null>");
    return RGF_SUCCESS;
}

extern "C" int __wrap_RF_RunBootStage(void)
{
    CHECK(CCD != NULL);
    CHECK(CCD->Player() == NULL);

    // Inherited constructor: missing playersetup.ini is an early return with
    // Actor == NULL. No level or avatar is involved.
    CPlayer *player = new CPlayer();
    CCD->thePlayer = player;
    CHECK(player->GetActor() == NULL);

    // The actor manager normally exists only after InitializeLevel; its
    // constructor and destructor touch no level state, so install one here.
    CHECK(CCD->ActorManager() == NULL);
    CActorManager *actors = new CActorManager();
    CCD->theActorManager = actors;

    CWeapon weapon;                       // early-returns on NULL entity set
    CHECK(weapon.GetCurrent() == -1);

    // First selection of a level: sentinel current weapon, valid slot.
    weapon.SetSlot(0, 0);
    weapon.SetWeapon(0);                  // D21: indexed WeaponD[-1] before the fix
    CHECK(weapon.GetCurrent() == 0);
    CHECK(motion_sets == 0);              // Sentinel path has no old animation to transfer.

    // Ordinary switch: the animation scan must still run for a real weapon.
    std::strcpy(weapon.WeaponD[0].Animations[3], "Idle");
    std::strcpy(weapon.WeaponD[1].Animations[3], "NeutralSwitch");
    weapon.SetSlot(1, 1);
    weapon.SetWeapon(1);
    CHECK(weapon.GetCurrent() == 1);
    CHECK(motion_sets == 1 && !std::strcmp(selected_motion, "NeutralSwitch"));

    // Sentinel again via the MAX_WEAPONS form.
    weapon.SetCurrent(MAX_WEAPONS);
    weapon.SetWeapon(0);
    CHECK(weapon.GetCurrent() == 0);
    CHECK(motion_sets == 1);

    CCD->theActorManager = NULL;
    delete actors;
    CCD->thePlayer = NULL;
    delete player;
    std::fprintf(stderr, "weapon sentinel: PASS\n");
    return 0;
}
