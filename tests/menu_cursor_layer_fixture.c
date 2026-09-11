/* Neutral layered menu/cursor submission through the active legacy driver. */
#include "Genesis.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <stdio.h>
#include <string.h>

static geBitmap *solid_bitmap(int width, int height, uint8 r, uint8 g, uint8 b)
{
    geBitmap *bitmap = geBitmap_Create(width, height, 1, GE_PIXELFORMAT_24BIT_RGB);
    geBitmap *lock = NULL;
    geBitmap_Info info;
    int x, y;
    if (!bitmap || !geBitmap_LockForWrite(bitmap, &lock, 0, 0) ||
        !geBitmap_GetInfo(lock, &info, NULL)) goto fail;
    for (y = 0; y < info.Height; ++y) {
        uint8 *row = (uint8 *)geBitmap_GetBits(lock) + y * info.Stride * 3;
        for (x = 0; x < info.Width; ++x) {
            row[x*3] = r; row[x*3+1] = g; row[x*3+2] = b;
        }
    }
    if (!geBitmap_UnLock(lock)) goto fail;
    return bitmap;
fail:
    if (lock) geBitmap_UnLock(lock);
    if (bitmap) geBitmap_Destroy(&bitmap);
    return NULL;
}

int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    geEngine *engine = NULL;
    geCamera *camera = NULL;
    geBitmap *button = NULL, *cursor = NULL;
    geDriver *driver;
    geDriver_Mode *mode;
    geRect camera_rect = {0, 319, 0, 239};
    geXForm3d transform;
    unsigned char pixel[3];
    int width, height, button_added = 0, cursor_added = 0, result = 1;
    if (argc != 2 || SDL_Init(SDL_INIT_VIDEO) != 0) goto done;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow("Neutral menu layer fixture", 0, 0, 320, 240,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) goto done;
    engine = geEngine_Create(window, "Neutral menu layer fixture", argv[1]);
    if (!engine) goto done;
    driver = geDriver_SystemGetNextDriver(geEngine_GetDriverSystem(engine), NULL);
    if (!driver) goto done;
    for (mode = geDriver_GetNextMode(driver, NULL); mode;
         mode = geDriver_GetNextMode(driver, mode)) {
        if (geDriver_ModeGetWidthHeight(mode, &width, &height) &&
            width == -1 && height == -1) break;
    }
    if (!mode || !geEngine_SetDriverAndModeNoSplash(engine, driver, mode)) goto done;
    camera = geCamera_Create(2.0f, &camera_rect);
    geXForm3d_SetIdentity(&transform);
    if (!camera || !geCamera_SetWorldSpaceXForm(camera, &transform)) goto done;
    button = solid_bitmap(32, 32, 255, 0, 0);
    cursor = solid_bitmap(8, 8, 0, 255, 0);
    if (!button || !cursor || !geEngine_AddBitmap(engine, button)) goto done;
    button_added = 1;
    if (!geEngine_AddBitmap(engine, cursor)) goto done;
    cursor_added = 1;
    if (!geEngine_BeginFrame(engine, camera, GE_TRUE) ||
        !geEngine_DrawBitmap(engine, button, NULL, 100, 100) ||
        !geEngine_DrawBitmap(engine, cursor, NULL, 110, 110)) goto done;
    glReadBuffer(GL_BACK);
    /* OpenGL readback is bottom-up; logical point (112,112) maps to y=127. */
    glReadPixels(112, 127, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    if (!geEngine_EndFrame(engine) || glGetError() != GL_NO_ERROR) goto done;
    if (pixel[0] > 4 || pixel[1] < 251 || pixel[2] > 4) {
        fprintf(stderr, "D25 layer: cursor did not cover button: %u/%u/%u\n",
            pixel[0], pixel[1], pixel[2]);
        goto done;
    }
    result = 0;
done:
    if (cursor_added) geEngine_RemoveBitmap(engine, cursor);
    if (cursor) geBitmap_Destroy(&cursor);
    if (button_added) geEngine_RemoveBitmap(engine, button);
    if (button) geBitmap_Destroy(&button);
    if (camera) geCamera_Destroy(&camera);
    if (engine) geEngine_Free(engine);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    if (!result) fprintf(stderr, "D25 layer: PASS cursor submitted above button\n");
    return result;
}
