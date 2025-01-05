// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	return true;
}

static bool on_btn_quit(view_t *view, SDL_Event *e, ui_t *ui) {
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_quit),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_lobby = {
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
