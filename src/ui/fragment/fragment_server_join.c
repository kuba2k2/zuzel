// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_btn_browse(view_t *view, SDL_Event *e, ui_t *ui) {
	ui->connection.type = UI_CONNECT_JOIN_BROWSE;
	free(ui->connection.server);
	ui->connection.server = NULL;
	free(ui->connection.key);
	ui->connection.key = NULL;

	ui_state_set_via(ui, UI_STATE_BROWSE, UI_STATE_CONNECTING);
	return true;
}

static bool on_input_key_change(view_t *view, SDL_Event *e, ui_t *ui) {
	view_t *button = gfx_view_find_by_id(fragment_server_join.views, "btn_key_ok");
	if (button == NULL)
		return false;

	button->is_disabled = strlen(view->data.input.value) != GAME_KEY_LEN;
	return true;
}

static bool on_btn_key_ok(view_t *view, SDL_Event *e, ui_t *ui) {
	view_t *input = gfx_view_find_by_id(fragment_server_join.views, "input_key");

	ui->connection.type = UI_CONNECT_JOIN_KEY;
	free(ui->connection.server);
	ui->connection.server = NULL;
	free(ui->connection.key);
	if (input != NULL)
		ui->connection.key = strdup(input->data.input.value);

	ui_state_set_via(ui, UI_STATE_BROWSE, UI_STATE_CONNECTING);
	return true;
}

static bool on_input_address_change(view_t *view, SDL_Event *e, ui_t *ui) {
	view_t *button = gfx_view_find_by_id(fragment_server_join.views, "btn_address_ok");
	if (button == NULL)
		return false;

	int dots	= 0;
	char *value = view->data.input.value;
	while (*value != '\0') {
		if (*value++ == '.')
			dots++;
	}

	button->is_disabled = dots != 3 || inet_addr(view->data.input.value) == 0xFFFFFFFF;
	return true;
}

static bool on_btn_address_ok(view_t *view, SDL_Event *e, ui_t *ui) {
	view_t *input = gfx_view_find_by_id(fragment_server_join.views, "input_address");

	ui->connection.type = UI_CONNECT_JOIN_ADDRESS;
	free(ui->connection.server);
	if (input != NULL)
		ui->connection.server = strdup(input->data.input.value);
	free(ui->connection.key);
	ui->connection.key = NULL;

	ui_state_set_via(ui, UI_STATE_BROWSE, UI_STATE_CONNECTING);
	return true;
}

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_set(ui, UI_STATE_MAIN);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_browse),
	GFX_VIEW_ON_EVENT(on_input_key_change),
	GFX_VIEW_ON_EVENT(on_btn_key_ok),
	GFX_VIEW_ON_EVENT(on_input_address_change),
	GFX_VIEW_ON_EVENT(on_btn_address_ok),
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_server_join = {
	.inflate_on_event = inflate_on_event,
};
