// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *menu) {
	menu->state = UI_STATE_MENU_MAIN;
	return true;
}

const view_inflate_on_event_t menu_server_join_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};
