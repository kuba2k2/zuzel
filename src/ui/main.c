// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#include "include.h"

int SDL_main(int argc, char *argv[]) {
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
		SDL_ERROR("SDL_CreateWindow()", ret = 1; goto quit);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
		SDL_ERROR("SDL_CreateRenderer()", ret = 1; goto free_window);
	SDL_RenderSetScale(renderer, (float)SETTINGS->screen.scale, (float)SETTINGS->screen.scale);

	game_t *game = NULL;

	menu_t *menu = menu_init(renderer);
	if (menu == NULL)
		LT_ERR(F, ret = 1; goto free_renderer, "Menu initialization failed");

	while (1) {
		if ((ret = menu_run(menu, &game)) != 0)
			LT_ERR(F, ret = 1; goto free_menu, "Menu function failed, ret=%d", ret);

		if (game == NULL)
			// menu exited successfully, quitting application
			break;

		race_t *race = race_init(renderer, game);
		if (race == NULL)
			LT_ERR(F, ret = 1; goto free_menu, "Race initialization failed");
		if ((ret = race_run(race)) != 0)
			LT_ERR(F, ret = 1; race_free(race); goto free_menu, "Race function failed, ret=%d", ret);
		race_free(race);
	}

free_menu:
	menu_free(menu);
free_renderer:
	SDL_DestroyRenderer(renderer);
free_window:
	SDL_DestroyWindow(window);
quit:
	SDL_Quit();
	return ret;
}
