// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_set(ui, UI_STATE_MAIN);
	net_server_stop();
	return true;
}

static bool on_btn_local(view_t *view, SDL_Event *e, ui_t *ui) {
	if (net_server_start() == NULL)
		LT_ERR(F, , "Couldn't start the server");
	return true;
}

const view_inflate_on_event_t fragment_server_new_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT(on_btn_local),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_server_new = {
	.inflate_on_event = fragment_server_new_on_event,
};
