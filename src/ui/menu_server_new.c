// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *menu) {
	menu->state = UI_STATE_MENU_MAIN;
	net_server_stop();
	return true;
}

static bool on_btn_local(view_t *view, SDL_Event *e, ui_t *menu) {
	if (net_server_start() == NULL)
		LT_ERR(F, , "Couldn't start the server");
	return true;
}

const view_inflate_on_event_t menu_server_new_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT(on_btn_local),
	GFX_VIEW_ON_EVENT_END(),
};
