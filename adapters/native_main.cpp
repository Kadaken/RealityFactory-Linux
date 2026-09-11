/* Native entry point. Automated profiles always use a hidden SDL window. */
#include "boot_watchdog.h" // Standard headers precede inherited min/max macros.
#include "RabidFramework.h"
#include "host_window.h"
#include "boot_game.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <exception>
#include <vector>
#include <stdint.h>

extern int WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
static bool configured_boot;
static std::string configured_level;

struct BootGameRun {
    struct NPCPosition { geActor *actor; geVec3d position; };
    RF_BootWatchdog watchdog{10000}; // Includes inherited GameLoop setup.
    int width = 0, height = 0, ticks = 0, step_calls = 0;
    bool failed = false, presented = false;
    std::vector<unsigned char> first, last;
    std::vector<NPCPosition> npc_start;
    std::chrono::steady_clock::time_point pace = std::chrono::steady_clock::now();
};
static BootGameRun *game_run;
bool RF_BootGameActive() { return game_run != NULL; }
float RF_BootGameStepMilliseconds()
{
    if (game_run) ++game_run->step_calls;
    return 1000.0f / 60.0f; // RF component Tick arguments are milliseconds.
}

static bool boot_game_fail(const char *reason)
{
    fprintf(stderr, "RF stage 5: %s; stop and inspect this boundary\n", reason);
    game_run->failed = true;
    return false;
}

bool RF_BootGameBeginTick()
{
    if (!game_run) return true;
    if (RF_HostQuitRequested()) return false;
    if (game_run->ticks >= RF_BOOT_GAME_TICKS)
        return boot_game_fail("tick limit reached without consumed quit");
    game_run->watchdog.arm(5000);
    game_run->step_calls = 0;
    game_run->presented = false;
    fprintf(stderr, "RF stage 5: tick=%d entering inherited input/DispatchTick/render/HUD\n", game_run->ticks + 1);
    if (!CCD->GetHasFocus() || !CCD->Inventory()->GetStopTime())
        return boot_game_fail("simulation inactive (logical focus or inventory stop-time)");
    return true;
}

static bool read_boot_pixels(int width, int height, std::vector<unsigned char> &pixels)
{
    GLint alignment, buffer;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_READ_BUFFER, &buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
    glReadBuffer(buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        fprintf(stderr, "RF boot frame: readback/preceding GL error 0x%x; inspect renderer before advancing\n", error);
    return error == GL_NO_ERROR;
}

bool RF_BootGameCapture()
{
    if (!game_run) return true;
    if (!read_boot_pixels(game_run->width, game_run->height, game_run->last))
        return boot_game_fail("frame readback/GL error");
    if (game_run->ticks == 0) game_run->first = game_run->last;
    return true;
}

bool RF_BootGameEndFrame(bool captured, int result)
{
    if (!game_run) return true;
    if (!captured || result != RGF_SUCCESS || glGetError() != GL_NO_ERROR)
        return boot_game_fail("capture or EndFrame failed");
    if (game_run->presented) return boot_game_fail("multiple main frames in one tick");
    game_run->presented = true;
    if (CCD->ChangeLevel() || !CCD->Player()->GetAlive() || CCD->GetPaused())
        return boot_game_fail("level transition/death/pause reached; outside bounded start-level test");
    return true;
}

bool RF_BootGameEndTick()
{
    if (!game_run) return true;
    if (!game_run->presented || game_run->step_calls != 1)
        return boot_game_fail("expected one DispatchTick time step and one completed frame");
    if (game_run->ticks == 0) {
        geEntity_EntitySet *set=geWorld_GetEntitySet(CCD->World(),"NonPlayerCharacter");
        for (geEntity *entity=set ? geEntity_EntitySetGetNextEntity(set,NULL) : NULL;
             entity; entity=geEntity_EntitySetGetNextEntity(set,entity)) {
            NonPlayerCharacter *npc=(NonPlayerCharacter *)geEntity_GetUserData(entity);
            Bot_Var *bot=npc ? (Bot_Var *)npc->DBot : NULL;
            BootGameRun::NPCPosition sample;
            if (bot && bot->Actor && CCD->ActorManager()->GetPosition(bot->Actor,&sample.position)==RGF_SUCCESS) {
                sample.actor=bot->Actor;
                game_run->npc_start.push_back(sample);
            }
        }
        fprintf(stderr,"RF stage 5: npc actor handles after first tick=%zu\n",game_run->npc_start.size());
    }
    if (game_run->ticks + 1 == RF_BOOT_GAME_TICKS) {
        size_t moved=0, retained=0;
        for (size_t i=0;i<game_run->npc_start.size();++i) {
            geVec3d position;
            if (CCD->ActorManager()->GetPosition(game_run->npc_start[i].actor,&position)==RGF_SUCCESS) {
                ++retained;
                if (geVec3d_DistanceBetween(&position,&game_run->npc_start[i].position)>0.001f) ++moved;
            }
        }
        fprintf(stderr,"RF stage 5: npc actors retained=%zu moved=%zu stationary=%zu\n",
            retained,moved,retained-moved);
    }
    ++game_run->ticks;
    fprintf(stderr, "RF stage 5: tick=%d completed dt_ms=%.6f GL_errors=0\n",
            game_run->ticks, (double)CCD->GetTicksGoneBy());
    RF_HostBootFrame(5);
    RF_HostPump(); // Consume the queued SDL_QUIT on the final tick.
    // Pace without spinning. Late frames do not trigger an unbounded catch-up.
    game_run->pace += std::chrono::nanoseconds(1000000000 / 60);
    const auto now = std::chrono::steady_clock::now();
    if (game_run->pace > now) std::this_thread::sleep_until(game_run->pace);
    else game_run->pace = now;
    return true;
}

static uint32_t boot_rgb_hash(const std::vector<unsigned char> &pixels)
{
    uint32_t hash = 2166136261u;
    for (unsigned char byte : pixels) hash = (hash ^ byte) * 16777619u;
    return hash;
}

static int run_boot_game()
{
    BootGameRun run;
    SDL_Window *window = (SDL_Window *)CCD->Engine()->WindowHandle();
    if (!window || !(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN)) return 1;
    SDL_GL_GetDrawableSize(window, &run.width, &run.height);
    if (run.width <= 0 || run.height <= 0 || run.width > 8192 || run.height > 8192 ||
        (size_t)run.width * run.height > 16777216) {
        fprintf(stderr, "RF stage 5: invalid/oversized framebuffer; check drawable dimensions\n");
        return 1;
    }
    run.first.resize((size_t)run.width * run.height * 3);
    run.last.resize(run.first.size());
    struct ActiveRun {
        explicit ActiveRun(BootGameRun &run) { game_run = &run; Joystick::setNeutralInput(true); }
        ~ActiveRun() { game_run = NULL; Joystick::setNeutralInput(false); }
    } active(run);
    fprintf(stderr, "RF stage 5: entering normal CRFMenu::GameLoop, target=%d, zero input\n", RF_BOOT_GAME_TICKS);
    CCD->MenuManager()->GameLoop(); // Setup, every manager, HUD, and post-frame logic stay upstream.
    if (run.failed || run.ticks != RF_BOOT_GAME_TICKS ||
        RF_HostBootFrameCount() != RF_BOOT_GAME_TICKS || !RF_HostQuitRequested()) {
        fprintf(stderr, "RF stage 5: incomplete ticks=%d/%d frames=%d quit=%d\n",
                run.ticks, RF_BOOT_GAME_TICKS, RF_HostBootFrameCount(), RF_HostQuitRequested());
        return 1;
    }
    size_t changed = 0;
    for (size_t i = 0; i < run.first.size(); i += 3)
        if (memcmp(&run.first[i], &run.last[i], 3)) ++changed;
    fprintf(stderr, "RF stage 5: ticks=%d frames=%d quit=%d first_rgb_fnv1a=%08x last_rgb_fnv1a=%08x changed_pixels=%zu GL_errors=0\n",
            run.ticks, RF_HostBootFrameCount(), RF_HostQuitRequested(),
            boot_rgb_hash(run.first), boot_rgb_hash(run.last), changed);
    return 0;
}

static int render_boot_world(void)
{
    CGenesisEngine *engine = CCD->Engine();
    if (!engine || !engine->World() || !CCD->CameraManager() || !CCD->Collision() ||
        !CCD->Liquids() || !CCD->Overlays()) {
        fprintf(stderr, "RF boot frame: world/camera/collision/liquid/overlay manager missing; InitializeLevel contract required\n");
        return 1;
    }
    SDL_Window *window = (SDL_Window *)engine->WindowHandle();
    int width = 0, height = 0;
    if (!window || !(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN)) {
        fprintf(stderr, "RF boot frame: hidden owned window required\n");
        return 1;
    }
    SDL_GL_GetDrawableSize(window, &width, &height);
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
        (size_t)width * height > 16777216) {
        fprintf(stderr, "RF boot frame: invalid/oversized evidence dimensions %d x %d\n", width, height);
        return 1;
    }
    const size_t size = (size_t)width * height * 3;
    std::vector<unsigned char> before(size), after(size);
    if (engine->BeginFrame() != RGF_SUCCESS) {
        fprintf(stderr, "RF boot frame: BeginFrame failed\n");
        return 1;
    }
    bool ok = read_boot_pixels(width, height, before);
    if (ok) {
        fprintf(stderr, "RF boot frame: entering CGenesisEngine::RenderWorld\n");
        ok = engine->RenderWorld() == RGF_SUCCESS;
        if (!ok) fprintf(stderr, "RF boot frame: CGenesisEngine::RenderWorld failed\n");
    }
    if (ok) ok = read_boot_pixels(width, height, after);
    if (engine->EndFrame() != RGF_SUCCESS) {
        fprintf(stderr, "RF boot frame: EndFrame failed\n");
        ok = false;
    }
    if (!ok) return 1;
    size_t changed = 0;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; i += 3) {
        if (before[i] != after[i] || before[i+1] != after[i+1] || before[i+2] != after[i+2]) ++changed;
        for (size_t c = 0; c < 3; ++c) hash = (hash ^ after[i+c]) * 16777619u;
    }
    const size_t center = ((size_t)(height/2) * width + width/2) * 3;
    fprintf(stderr, "RF boot frame: %dx%d changed_pixels=%zu rgb_fnv1a=%08x center=%u/%u/%u\n",
        width, height, changed, hash, after[center], after[center+1], after[center+2]);
    if (!changed) {
        fprintf(stderr, "RF boot frame: no visible change from BeginFrame baseline; not claiming a rendered world\n");
        return 1;
    }
    return 0;
}

extern "C" int RF_RunBootStage(void)
{
    const int stage = RF_HostBootStage();
    if (stage == 1) return RF_RunNeutralFrame();
    fprintf(stderr, "RF boot stage %d: full InitializeCommon returned\n", stage);
    if (!(SDL_GetWindowFlags((SDL_Window *)CCD->Engine()->WindowHandle()) & SDL_WINDOW_HIDDEN)) {
        fprintf(stderr, "RF boot: window unexpectedly visible\n");
        return 1;
    }
    if (stage == 2) {
        // Real-data measurement observes only calls made by upstream startup.
        // The synthetic rejection checks belong to the neutral regression.
        if (configured_boot) return 0;
        int tracks[] = {1};
        if (CCD->AudioManager()->Start(0) != RGF_UNIMPLEMENTED ||
            CCD->CDPlayer()->Play(1, false) != RGF_UNIMPLEMENTED ||
            CCD->CDPlayer()->PlaySequence(1, tracks) != RGF_UNIMPLEMENTED ||
            CCD->CDPlayer()->IsPlaying() ||
            CCD->MIDIPlayer()->Play("neutral.mid", false) != RGF_FAILURE ||
            CCD->MIDIPlayer()->IsPlaying()) {
            fprintf(stderr, "RF boot stage 2: unavailable media reported success\n");
            return 1;
        }
        fprintf(stderr, "RF boot stage 2: unavailable media rejected playback requests\n");
        return 0;
    }
    if (stage == 3) {
        CCD->MenuManager()->DoMenu(configured_boot ? configured_level.c_str() : "neutral.bsp");
        fprintf(stderr, "RF boot stage 3: menu frames=%d, quit=%d\n",
                RF_HostBootFrameCount(), RF_HostQuitRequested());
        return RF_HostBootFrameCount() != 1 || !RF_HostQuitRequested();
    }
    const int loaded = CCD->InitializeLevel(configured_boot ? configured_level.c_str() : "neutral.bsp");
    if (loaded != 0) {
        fprintf(stderr, "RF boot stage %d: CCommonData::InitializeLevel failed (%d); no gameplay/render pass claimed\n", stage, loaded);
        return 1;
    }
    fprintf(stderr, "RF boot stage %d: InitializeLevel returned success\n", stage);
    if (stage == 4) {
        if (render_boot_world() != 0) return 1;
        RF_HostBootFrame(stage);
        RF_HostPump();
        fprintf(stderr, "RF boot stage 4: world frames=%d, quit=%d; no gameplay simulation claimed\n",
            RF_HostBootFrameCount(), RF_HostQuitRequested());
        return RF_HostBootFrameCount() != 1 || !RF_HostQuitRequested();
    }
    return run_boot_game();
}

extern "C" int RF_RunNeutralFrame(void)
{
    CGenesisEngine *engine = CCD->Engine();
    fprintf(stderr, "RF neutral boot: CCommonData/engine/camera initialized\n");
    if (!engine->LoadLevel("neutral.bsp")) return 1;
    geCamera *camera = CCD->CameraManager()->Camera();
    geXForm3d transform;
    geXForm3d_SetIdentity(&transform);
    if (!geCamera_SetWorldSpaceXForm(camera, &transform)) return 1;
    if (!(SDL_GetWindowFlags((SDL_Window *)engine->WindowHandle()) & SDL_WINDOW_HIDDEN)) {
        fprintf(stderr, "RF neutral boot: window unexpectedly visible\n");
        return 1;
    }
    if (engine->BeginFrame() != RGF_SUCCESS) return 1;
    // A content-free frame has no initialized game collision/liquid managers.
    // Use the actual engine render API, not CGenesisEngine's gameplay overlays.
    if (!geEngine_RenderWorld(engine->Engine(), engine->World(), camera, 0)) return 1;
    unsigned char pixel[3];
    glReadBuffer(GL_BACK);
    glReadPixels(engine->Width()/2, engine->Height()/2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    const bool gl_ok = glGetError() == GL_NO_ERROR;
    if (engine->EndFrame() != RGF_SUCCESS) return 1;
    fprintf(stderr, "RF first frame: RGB %u %u %u\n", pixel[0], pixel[1], pixel[2]);
    return !gl_ok || abs((int)pixel[0]-12)>2 || abs((int)pixel[1]-110)>2 || abs((int)pixel[2]-24)>2;
}

int main(int argc, char **argv)
{
    bool human = false, neutral = false;
    int stage = 0;
    std::string command;
    std::string config_directory;
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "--human-launch")) human = true;
        else if (!strcmp(argv[i], "--neutral-frame")) { neutral = true; stage = 1; }
        else if (!strncmp(argv[i], "--boot-stage=", 13) && strlen(argv[i]) == 14 &&
                 argv[i][13] >= '1' && argv[i][13] <= '5') {
            neutral = true; stage = argv[i][13] - '0';
        }
        else if (!strncmp(argv[i], "--config-dir=", 13) && argv[i][13]) {
            if (!config_directory.empty()) return 2;
            config_directory = argv[i] + 13;
        }
        else if (!strcmp(argv[i], "--map") && i+1<argc) {
            if (!argv[i+1][0]) { fprintf(stderr, "RF: --map requires a nonempty level name\n"); return 2; }
            command = "-map "; command += argv[++i];
        } else {
            fprintf(stderr, "usage: realityfactory_linux --boot-stage=1..5 [--config-dir=copy] | --neutral-frame | --human-launch [--map level]\n");
            return 2;
        }
    }
    if ((!human && !neutral) || (human && neutral) || command.size()>240 ||
        (neutral && !command.empty())) {
        fprintf(stderr, "RF: choose hidden --neutral-frame or explicit --human-launch, not both\n");
        return 2;
    }
    if (!config_directory.empty()) {
        if (human || stage < 2 || stage > 5 || !command.empty()) {
            fprintf(stderr, "RF: --config-dir requires hidden --boot-stage=2..5 without --map\n");
            return 2;
        }
        char resolved[PATH_MAX];
        if (!realpath(config_directory.c_str(), resolved) || chdir(resolved) != 0) {
            fprintf(stderr, "RF: cannot resolve/open config directory: %s\n", strerror(errno));
            return 2;
        }
        fprintf(stderr, "RF measurement: resolved config dir: %s\n", resolved);
        configured_boot = true;
        // Same startlevel tokenization as InitializeCommon, without injecting
        // '-map neutral' (which would override the authored startup level).
        FILE *config = fopen("RealityFactory.ini", "rt");
        if (!config) {
            fprintf(stderr, "RF: missing exact-case RealityFactory.ini in resolved config directory\n");
            return 2;
        }
        char line[132];
        while (fgets(line, sizeof(line), config)) {
            if (line[0] == ';' || strlen(line) <= 5) continue;
            char *key = strtok(line, " =");
            char *value = strtok(NULL, " \n");
            if (key && value && !stricmp(key, "startlevel")) configured_level = value;
        }
        fclose(config);
    }
    if (neutral && !configured_boot) command = "-map neutral";
    RF_HostConfigure(human);
    RF_HostSetNeutralBoot(neutral);
    RF_HostSetBootStage(stage);
    char mutable_command[256];
    strcpy(mutable_command, command.c_str());
    try {
        const int result = WinMain(NULL, NULL, mutable_command, 0);
        fprintf(stderr, "RF native exit: %d\n", result);
        return result;
    } catch (const std::exception &error) {
        fprintf(stderr, "RF native startup failed: %s\n", error.what());
        if (CCD) { delete CCD; CCD = NULL; }
        return 1;
    }
}
