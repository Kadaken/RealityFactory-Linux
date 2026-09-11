/* Measurement-only wrappers for the RF-12 round-15 input/weapon pass. */
#include "RabidFramework.h"
#include "host_window.h"
#include "input_compat.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstring>
#include <vector>

static int frame_index;
static int consumed[32];
static int consumed_count;
static geWorld *transition_world;
static bool transition_requested;

extern "C" bool __real__ZN11CCommonData15HandleGameInputEv(CCommonData *);
extern "C" int __real__ZN6CInput13GetFirstInputEv(CInput *);
extern "C" int __real__ZN6CInput12GetNextInputEv(CInput *);
extern "C" bool __real__Z19RF_BootGameEndFramebi(bool, int);
extern "C" bool __real__Z18RF_BootGameCapturev(void);
extern "C" geBoolean __real_geWorld_SetActorFlags(geWorld *, geActor *, uint32);
extern "C" bool __wrap__Z18RF_BootGameCapturev(void)
{
    const bool result = __real__Z18RF_BootGameCapturev();
    static int capture_index;
    if (result && (capture_index == 0 || capture_index == 5)) {
        SDL_Window *window = (SDL_Window *)CCD->Engine()->WindowHandle();
        int width = 0, height = 0;
        SDL_GL_GetDrawableSize(window, &width, &height);
        std::vector<unsigned char> pixels((size_t)width * height * 3);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        uint32_t hash = 2166136261u;
        for (unsigned char byte : pixels) hash = (hash ^ byte) * 16777619u;
        CWeapon *weapons = CCD->Weapons();
        geXForm3d actor_xform;
        geVec3d camera_point = {0, 0, 0};
        geExtBox render_box;
        geBoolean box_enabled = GE_FALSE;
        float min_z = 0.0f, max_z = 0.0f;
        int pose = 0;
        if (weapons && weapons->CurrentWeapon >= 0 && weapons->CurrentWeapon < MAX_WEAPONS) {
            geActor *actor = weapons->WeaponD[weapons->CurrentWeapon].VActor;
            pose = actor && geActor_GetBoneTransform(actor, RootBoneName(actor), &actor_xform);
            if (pose)
                geXForm3d_Transform(geCamera_GetCameraSpaceXForm(CCD->CameraManager()->Camera()),
                                    &actor_xform.Translation, &camera_point);
            if (actor && geActor_GetRenderHintExtBox(actor, &render_box, &box_enabled) && box_enabled) {
                for (int corner = 0; corner < 8; ++corner) {
                    geVec3d point = {
                        corner & 1 ? render_box.Max.X : render_box.Min.X,
                        corner & 2 ? render_box.Max.Y : render_box.Min.Y,
                        corner & 4 ? render_box.Max.Z : render_box.Min.Z
                    };
                    geVec3d transformed;
                    geXForm3d_Transform(geCamera_GetCameraSpaceXForm(CCD->CameraManager()->Camera()),
                                        &point, &transformed);
                    if (!corner || transformed.Z < min_z) min_z = transformed.Z;
                    if (!corner || transformed.Z > max_z) max_z = transformed.Z;
                }
            }
        }
        std::fprintf(stderr,
            "RF D28: capture=%d rgb_fnv1a=%08x pose=%d camera_position=%.3f/%.3f/%.3f box=%d camera_z=%.3f..%.3f\n",
            capture_index, hash, pose, (double)camera_point.X, (double)camera_point.Y,
            (double)camera_point.Z, box_enabled, (double)min_z, (double)max_z);
    }
    ++capture_index;
    return result;
}

extern "C" geBoolean __wrap_geWorld_SetActorFlags(geWorld *world, geActor *actor, uint32 flags)
{
    return __real_geWorld_SetActorFlags(world, actor, flags);
}
static int remember(int action)
{
    if (consumed_count < (int)(sizeof(consumed) / sizeof(consumed[0])))
        consumed[consumed_count++] = action;
    return action;
}

extern "C" int __wrap__ZN6CInput13GetFirstInputEv(CInput *input)
{
    return remember(__real__ZN6CInput13GetFirstInputEv(input));
}

extern "C" int __wrap__ZN6CInput12GetNextInputEv(CInput *input)
{
    return remember(__real__ZN6CInput12GetNextInputEv(input));
}

static void push_key(Uint32 type, SDL_Scancode scancode)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = type;
    event.key.type = type;
    event.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.scancode = scancode;
    event.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
    RF_InputEvent(&event);
}

static void print_weapon_snapshot()
{
    CWeapon *weapons = CCD->Weapons();
    CPersistentAttributes *inventory =
        CCD->ActorManager()->Inventory(CCD->Player()->GetActor());
    std::fprintf(stderr, "RF D35: current=%d\n", weapons->CurrentWeapon);
    for (int index = 0; index < MAX_WEAPONS; ++index) {
        DefaultWeapons &item = weapons->WeaponD[index];
        if (!item.active && item.Name[0] == '\0') continue;
        std::fprintf(stderr,
            "RF D35: index=%d active=%d slot=%d has=%d value=%d view_def=%d view_actor=%d view_scale=%.6f view_offset=%.3f/%.3f/%.3f player_actor=%d\n",
            index, item.active ? 1 : 0, item.Slot,
            inventory->Has(item.Name) == RGF_SUCCESS ? 1 : 0,
            inventory->Has(item.Name) == RGF_SUCCESS ? inventory->Value(item.Name) : -1,
            item.VActorDef != NULL, item.VActor != NULL, (double)item.VScale,
            (double)item.VActorOffset.X, (double)item.VActorOffset.Y,
            (double)item.VActorOffset.Z, item.PActor != NULL);
    }
    for (int slot = 0; slot < MAX_WEAPONS; ++slot)
        if (weapons->Slot[slot] != -1)
            std::fprintf(stderr, "RF D35: slot=%d index=%d\n", slot, weapons->Slot[slot]);
}

static void print_transition_state(const char *phase)
{
    geActor *actor = CCD->Player()->GetActor();
    geVec3d position = CCD->Player()->Position();
    geExtBox box;
    int forces = 0;
    for (int index = 0; index < 4; ++index)
        forces += CCD->ActorManager()->ForceActive(actor, index) == GE_TRUE;
    const int solid = CCD->ActorManager()->GetBoundingBox(actor, &box) == RGF_SUCCESS &&
        CCD->Collision()->CheckSolid(&position, &box, actor);
    std::fprintf(stderr,
        "RF D27: phase=%s moving=%d held=%d forces=%d solid=%d position=%.3f/%.3f/%.3f\n",
        phase, CCD->Player()->GetMoving(),
        !!GetAsyncKeyState('W') + !!GetAsyncKeyState('S') +
        !!GetAsyncKeyState('A') + !!GetAsyncKeyState('D'),
        forces, solid, (double)position.X, (double)position.Y, (double)position.Z);
}

extern "C" bool __wrap__Z19RF_BootGameEndFramebi(bool captured, int result_code)
{
    const bool result = __real__Z19RF_BootGameEndFramebi(captured, result_code);
    static bool compared_view_actor;
    if (result && !compared_view_actor) {
        CWeapon *weapons = CCD->Weapons();
        geActor *actor = weapons && weapons->CurrentWeapon >= 0 &&
            weapons->CurrentWeapon < MAX_WEAPONS
            ? weapons->WeaponD[weapons->CurrentWeapon].VActor : NULL;
        SDL_Window *window = (SDL_Window *)CCD->Engine()->WindowHandle();
        int width = 0, height = 0;
        SDL_GL_GetDrawableSize(window, &width, &height);
        std::vector<unsigned char> visible((size_t)width * height * 3);
        std::vector<unsigned char> hidden(visible.size());
        auto render = [&](std::vector<unsigned char> &pixels) {
            if (CCD->Engine()->BeginFrame() != RGF_SUCCESS) return false;
            if (CCD->Engine()->RenderWorld() != RGF_SUCCESS) return false;
            glReadBuffer(GL_BACK);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            return CCD->Engine()->EndFrame() == RGF_SUCCESS;
        };
        __real_geWorld_SetActorFlags(CCD->World(), actor, GE_ACTOR_RENDER_NORMAL);
        const bool visible_ok = render(visible);
        __real_geWorld_SetActorFlags(CCD->World(), actor, 0);
        const bool hidden_ok = render(hidden);
        __real_geWorld_SetActorFlags(CCD->World(), actor, GE_ACTOR_RENDER_NORMAL);
        size_t changed = 0;
        if (visible_ok && hidden_ok)
            for (size_t offset = 0; offset < visible.size(); offset += 3)
                changed += !!std::memcmp(&visible[offset], &hidden[offset], 3);
        std::fprintf(stderr, "RF D28: paired_render_ok=%d changed_pixels=%zu\n",
                     visible_ok && hidden_ok, changed);
        compared_view_actor = true;
    }
    if (!result || transition_requested || frame_index < 6) return result;
    geEntity_EntitySet *set = geWorld_GetEntitySet(CCD->World(), "ChangeLevel");
    for (geEntity *entity = set ? geEntity_EntitySetGetNextEntity(set, NULL) : NULL;
         entity; entity = geEntity_EntitySetGetNextEntity(set, entity)) {
        ChangeLevel *item = (ChangeLevel *)geEntity_GetUserData(entity);
        if (item && !EffectC_IsStringNull(item->szNewLevel)) {
            print_transition_state("before");
            transition_world = CCD->World();
            CCD->SetChangeLevelData(item);
            CCD->SetChangeLevel(true);
            transition_requested = true;
            std::fprintf(stderr, "RF D27: transition_requested=1\n");
            break;
        }
    }
    return result;
}

extern "C" bool __wrap__ZN11CCommonData15HandleGameInputEv(CCommonData *common)
{
    static const SDL_Scancode scans[] = {
        SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D
    };
    static const int virtual_keys[] = {'W', 'S', 'A', 'D'};
    if (frame_index == 1) print_weapon_snapshot();

    consumed_count = 0;
    if (frame_index < 4) {
        const SDL_Scancode scan = scans[frame_index];
        push_key(SDL_KEYDOWN, scan);
        int count = 0;
        const Uint8 *state = SDL_GetKeyboardState(&count);
        const int async_state = GetAsyncKeyState(virtual_keys[frame_index]) != 0;
        const int before_count = common->Input()->m_KeyStackCount;
        const bool result = __real__ZN11CCommonData15HandleGameInputEv(common);
        const int after_count = common->Input()->m_KeyStackCount;
        std::fprintf(stderr,
            "RF D26: key=%c vk=%d scan=%d sdl=%d async=%d stack_before=%d stack_after=%d actions=",
            virtual_keys[frame_index], virtual_keys[frame_index], (int)scan,
            scan < count ? (int)state[scan] : -1, async_state,
            before_count, after_count);
        for (int i = 0; i < consumed_count; ++i)
            std::fprintf(stderr, "%s%d", i ? "," : "", consumed[i]);
        std::fprintf(stderr, " moving=%d\n", common->Player()->GetMoving());
        push_key(SDL_KEYUP, scan);
        ++frame_index;
        return result;
    }

    if (frame_index == 4 || frame_index == 5) {
        const int before = common->Weapons()->GetCurrent();
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_MOUSEWHEEL;
        event.wheel.y = frame_index == 4 ? -1 : 1;
        RF_InputEvent(&event);
        const bool result = __real__ZN11CCommonData15HandleGameInputEv(common);
        std::fprintf(stderr, "RF D35: wheel_y=%d actions=", event.wheel.y);
        for (int i = 0; i < consumed_count; ++i)
            std::fprintf(stderr, "%s%d", i ? "," : "", consumed[i]);
        std::fprintf(stderr, " before=%d after=%d\n", before, common->Weapons()->GetCurrent());
        ++frame_index;
        return result;
    }
    if (transition_requested && transition_world != CCD->World()) {
        print_transition_state("after");
        std::fprintf(stderr, "RF D27: world_changed=1\n");
        transition_world = CCD->World();
        ++frame_index;
    }
    return __real__ZN11CCommonData15HandleGameInputEv(common);
}
