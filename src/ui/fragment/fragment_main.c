// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_btn_quick_play(view_t *view, SDL_Event *e, ui_t *ui) {
	return true;
}

static bool on_btn_new_game(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_set(ui, UI_STATE_SERVER_NEW);
	return true;
}

static bool on_btn_join_game(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_set(ui, UI_STATE_SERVER_JOIN);
	return true;
}

static bool on_btn_exit(view_t *view, SDL_Event *e, ui_t *i) {
	SDL_Event quit = {
		.quit.type		= SDL_QUIT,
		.quit.timestamp = SDL_GetTicks(),
	};
	SDL_PushEvent(&quit);
	return true;
}

const view_inflate_on_event_t fragment_main_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_quick_play),
	GFX_VIEW_ON_EVENT(on_btn_new_game),
	GFX_VIEW_ON_EVENT(on_btn_join_game),
	GFX_VIEW_ON_EVENT(on_btn_exit),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_main = {
	.inflate_on_event = fragment_main_on_event,
};
