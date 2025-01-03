// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_btn_public(view_t *view, SDL_Event *e, ui_t *ui) {
	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_NEW_PUBLIC;
	ui->connection.address = strdup(SETTINGS->public_server_address);
	ui->connection.use_tls = true;
	ui->connection.key	   = NULL;

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_private(view_t *view, SDL_Event *e, ui_t *ui) {
	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_NEW_PRIVATE;
	ui->connection.address = strdup(SETTINGS->public_server_address);
	ui->connection.use_tls = true;
	ui->connection.key	   = NULL;

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_local(view_t *view, SDL_Event *e, ui_t *ui) {
	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_NEW_LOCAL;
	ui->connection.address = strdup("127.0.0.1");
	ui->connection.use_tls = false;
	ui->connection.key	   = NULL;

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_set(ui, UI_STATE_MAIN);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_public),
	GFX_VIEW_ON_EVENT(on_btn_private),
	GFX_VIEW_ON_EVENT(on_btn_local),
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_server_new = {
	.inflate_on_event = inflate_on_event,
};
