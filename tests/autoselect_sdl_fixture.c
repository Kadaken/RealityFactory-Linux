#include "rf_platform_compat.h"
#include "AutoSelect.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static int require_true(int condition, const char *message)
{
	if(condition)
		return 1;

	fprintf(stderr, "autoselect fixture: %s\n", message);
	return 0;
}

int main(void)
{
	ModeList Modes[3];
	SDL_DisplayMode DesktopMode;
	int Selection = -1;

	memset(Modes, 0, sizeof(Modes));
	Modes[0].Width = 640;
	Modes[0].Height = 480;
	Modes[0].Evaluation = MODELIST_EVALUATED_UNDESIRABLE;
	Modes[1].Width = 800;
	Modes[1].Height = 600;
	Modes[1].Evaluation = MODELIST_EVALUATED_OK;
	Modes[2].Width = 320;
	Modes[2].Height = 240;
	Modes[2].Evaluation = MODELIST_EVALUATED_OK;

	SDL_Quit();
	if(!require_true(DrvList_PickDriver(NULL, NULL, Modes, 3, &Selection),
					 "pre-SDL selection failed") ||
	   !require_true(Selection == 1,
					 "pre-SDL selection did not preserve sorted-list policy"))
		return 1;

	Modes[0].Evaluation = MODELIST_EVALUATED_TRIED_FAILED;
	Modes[1].Evaluation = MODELIST_EVALUATED_TRIED_FAILED;
	Modes[2].Evaluation = MODELIST_EVALUATED_TRIED_FAILED;
	Selection = -1;
	if(!require_true(!DrvList_PickDriver(NULL, NULL, Modes, 3, &Selection),
					 "failed modes were accepted"))
		return 1;

	if(SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr, "autoselect fixture: SDL video init failed: %s\n",
				SDL_GetError());
		return 1;
	}
	if(SDL_GetCurrentDisplayMode(0, &DesktopMode) != 0)
	{
		fprintf(stderr, "autoselect fixture: no dummy desktop mode: %s\n",
				SDL_GetError());
		SDL_Quit();
		return 1;
	}

	memset(Modes, 0, sizeof(Modes));
	Modes[0].Width = DesktopMode.w > 1 ? DesktopMode.w - 1 : DesktopMode.w + 1;
	Modes[0].Height = DesktopMode.h;
	Modes[0].Evaluation = MODELIST_EVALUATED_OK;
	Modes[1].Width = DesktopMode.w;
	Modes[1].Height = DesktopMode.h;
	Modes[1].Evaluation = MODELIST_EVALUATED_OK;
	Selection = -1;
	if(!require_true(DrvList_PickDriver(NULL, NULL, Modes, 2, &Selection),
					 "SDL display-backed selection failed") ||
	   !require_true(Selection == 1,
					 "SDL display-backed selection did not choose the desktop mode"))
	{
		SDL_Quit();
		return 1;
	}

	{
		SDL_Window *window = SDL_CreateWindow("Neutral resize", 0, 0, 320, 240, SDL_WINDOW_HIDDEN);
		int width = 0, height = 0;
		if (!window) { SDL_Quit(); return 1; }
		ResetMainWindow(window, 400, 300);
		SDL_GetWindowSize(window, &width, &height);
		if (!require_true(width == 400 && height == 300 &&
			(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN), "resize or hidden-window contract failed")) {
			SDL_DestroyWindow(window); SDL_Quit(); return 1;
		}
		SDL_DestroyWindow(window);
	}
	SDL_Quit();
	return 0;
}
