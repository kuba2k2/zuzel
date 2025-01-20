// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#include "include.h"

#include <SDL_main.h>

int main(int argc, char *argv[]) {
	srand(time(NULL));

	MALLOC(SETTINGS, sizeof(*SETTINGS), return 1);
	settings_load();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		SDL_ERROR("SDL_Init()", return 1);

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8)
	// Disable compositor bypass
	if (SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0") == false)
		SDL_ERROR("SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR)", );
#endif

	int ret = 0;

	SDL_Window *window = SDL_CreateWindow(
		"Żużel",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SETTINGS->screen.width * SETTINGS->screen.scale,
		SETTINGS->screen.height * SETTINGS->screen.scale,
		SDL_WINDOW_SHOWN
	);
	if (window == NULL)
		SDL_ERROR("SDL_CreateWindow()", ret = 2; goto quit);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
		SDL_ERROR("SDL_CreateRenderer()", ret = 3; goto free_window);
	SDL_RenderSetScale(renderer, (float)SETTINGS->screen.scale, (float)SETTINGS->screen.scale);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	ui_t *ui = ui_init(window, renderer);
	if (ui == NULL)
		LT_ERR(F, ret = 4; goto free_renderer, "UI initialization failed");

	ret = ui_run(ui);
	if (ret != 0)
		LT_ERR(F, , "UI function failed, ret=%d", ret);

	ui_free(ui);
free_renderer:
	SDL_DestroyRenderer(renderer);
free_window:
	SDL_DestroyWindow(window);
quit:
	SDL_Quit();
	return ret;
}
