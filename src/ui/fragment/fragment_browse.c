// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "fragment.h"

#define PER_PAGE 5

static void on_error(ui_t *ui);

static void get_game_list(ui_t *ui, int page) {
	pkt_game_list_t pkt = {
		.hdr.type	 = PKT_GAME_LIST,
		.page		 = page,
		.per_page	 = PER_PAGE,
		.total_count = 0,
	};
	if (net_pkt_send(ui->client, (pkt_t *)&pkt) != NET_ERR_OK)
		on_error(ui);
}

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	get_game_list(ui, 0);
	return true;
}

static void on_error(ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_error(ui);
}

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->client = NULL;
	net_client_stop();
	net_server_stop();
	ui_state_prev(ui);
	return true;
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_browse = {
	.on_show		  = on_show,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
