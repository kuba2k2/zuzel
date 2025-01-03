// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	bool result = false;
	char message[128];

	// start the network stack and update UI message
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		strcpy(message, "Connecting to local game server...");
		result = net_server_start();
	} else if (ui->connection.type == UI_CONNECT_JOIN_ADDRESS) {
		sprintf(message, "Connecting to %s...", ui->connection.server);
		result = true;
	} else {
		strcpy(message, "Connecting to public game server...");
		result = true;
	}

	if (!result) {
		ui_state_prev(ui);
		ui_state_set(ui, UI_STATE_ERROR);
		return false;
	}

	gfx_view_set_text(gfx_view_find_by_id(fragment->views, "message"), message);
	return true;
}

static bool on_btn_cancel(view_t *view, SDL_Event *e, ui_t *ui) {
	net_server_stop();
	ui_state_prev(ui);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_cancel),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_connecting = {
	.on_show		  = on_show,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
