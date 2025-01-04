// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	net_t *net = NULL;
	char message[128];

	lt_log_clear_errors();

	// start the network stack and update UI message
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		strcpy(message, "Connecting to local game server...");
		net = net_server_start();
	} else if (ui->connection.type == UI_CONNECT_JOIN_ADDRESS) {
		sprintf(message, "Connecting to %s...", ui->connection.address);
		net = net_client_start(ui->connection.address, ui->connection.use_tls);
	} else {
		strcpy(message, "Connecting to public game server...");
		net = net_client_start(ui->connection.address, ui->connection.use_tls);
	}

	if (net == NULL) {
		ui_state_error(ui);
		return false;
	}

	gfx_view_set_text(gfx_view_find_by_id(fragment->views, "message"), message);
	return true;
}

static bool on_btn_cancel(view_t *view, SDL_Event *e, ui_t *ui) {
	net_client_stop();
	net_server_stop();
	ui_state_prev(ui);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	switch (e->type) {
		case SDL_USEREVENT_SERVER:
			if (e->user.code == false)
				ui_state_error(ui);
			else if (net_client_start(ui->connection.address, ui->connection.use_tls) == NULL)
				ui_state_error(ui);
			return true;
	}
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
