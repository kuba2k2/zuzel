// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	void *result = NULL;
	char message[128];

	lt_log_clear_errors();

	// start the network stack and update UI message
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		strcpy(message, "Connecting to local game server...");
		result = net_server_start();
	} else if (ui->connection.type == UI_CONNECT_JOIN_ADDRESS) {
		sprintf(message, "Connecting to %s...", ui->connection.address);
		result = ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
	} else {
		strcpy(message, "Connecting to public game server...");
		result = ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
	}

	if (result == NULL) {
		ui_state_error(ui);
		return false;
	}

	gfx_view_set_text(gfx_view_find_by_id(fragment->views, "message"), message);
	return true;
}

static void on_connected(ui_t *ui) {
	if (ui->client == NULL)
		return;
	LT_I("Got connected event");
}

static void on_packet(ui_t *ui, pkt_t *pkt) {
	if (ui->client == NULL)
		return;
	LT_I("Got packet on SDL");
}

static bool on_btn_cancel(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_prev(ui);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	switch (e->type) {
		case SDL_USEREVENT_SERVER:
			if (e->user.code == false) {
				ui_state_error(ui);
				return true;
			}
			ui->client = net_client_start(ui->connection.address, ui->connection.use_tls);
			if (ui->client == NULL)
				ui_state_error(ui);
			return true;

		case SDL_USEREVENT_CLIENT:
			if (e->user.code == false)
				ui_state_error(ui);
			else
				on_connected(ui);
			return true;

		case SDL_USEREVENT_PACKET:
			on_packet(ui, e->user.data1);
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
