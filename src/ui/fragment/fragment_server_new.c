// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_btn_public(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->connection.type = UI_CONNECT_NEW_PUBLIC;
	free(ui->connection.server);
	ui->connection.server = NULL;
	free(ui->connection.key);
	ui->connection.key = NULL;

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_private(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->connection.type = UI_CONNECT_NEW_PRIVATE;
	free(ui->connection.server);
	ui->connection.server = NULL;
	free(ui->connection.key);
	ui->connection.key = NULL;

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_local(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->connection.type = UI_CONNECT_NEW_LOCAL;
	free(ui->connection.server);
	ui->connection.server = NULL;
	free(ui->connection.key);
	ui->connection.key = NULL;

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
