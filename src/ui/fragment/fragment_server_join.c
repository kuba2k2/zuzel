// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "fragment.h"

static bool on_input_address_change(view_t *view, SDL_Event *e, ui_t *ui);

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	view_t *input_address;
	GFX_VIEW_BIND(fragment->views, input_address, return false);
	if (SETTINGS->last_join_address != NULL)
		strncpy2(input_address->data.input.value, SETTINGS->last_join_address, input_address->data.input.max_length);
	on_input_address_change(input_address, e, ui);
	return true;
}

static bool on_btn_browse(view_t *view, SDL_Event *e, ui_t *ui) {
	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_JOIN_BROWSE;
	ui->connection.address = strdup(SETTINGS->public_server_address);
	ui->connection.use_tls = true;
	ui->connection.key	   = NULL;

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
	if (input == NULL)
		return false;

	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_JOIN_KEY;
	ui->connection.address = strdup(SETTINGS->public_server_address);
	ui->connection.use_tls = true;
	ui->connection.key	   = strdup(input->data.input.value);

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
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
	if (input == NULL)
		return false;

	FREE_NULL(ui->connection.address);
	FREE_NULL(ui->connection.key);
	ui->connection.type	   = UI_CONNECT_JOIN_ADDRESS;
	ui->connection.address = strdup(input->data.input.value);
	ui->connection.use_tls = false;
	ui->connection.key	   = NULL;

	FREE_NULL(SETTINGS->last_join_address);
	SETTINGS->last_join_address = strdup(input->data.input.value);
	settings_save();

	ui_state_set_via(ui, UI_STATE_LOBBY, UI_STATE_CONNECTING);
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
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
