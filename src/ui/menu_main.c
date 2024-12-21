// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

static bool on_btn_quick_play(view_t *view, SDL_Event *e, ui_t *menu) {
	return true;
}

static bool on_btn_new_game(view_t *view, SDL_Event *e, ui_t *menu) {
	menu->state = UI_STATE_MENU_SERVER_NEW;
	return true;
}

static bool on_btn_join_game(view_t *view, SDL_Event *e, ui_t *menu) {
	menu->state = UI_STATE_MENU_SERVER_JOIN;
	return true;
}

static bool on_btn_exit(view_t *view, SDL_Event *e, ui_t *menu) {
	SDL_Event quit = {
		.quit.type		= SDL_QUIT,
		.quit.timestamp = SDL_GetTicks(),
	};
	SDL_PushEvent(&quit);
	return true;
}

const view_inflate_on_event_t menu_main_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_quick_play),
	GFX_VIEW_ON_EVENT(on_btn_new_game),
	GFX_VIEW_ON_EVENT(on_btn_join_game),
	GFX_VIEW_ON_EVENT(on_btn_exit),
	GFX_VIEW_ON_EVENT_END(),
};
