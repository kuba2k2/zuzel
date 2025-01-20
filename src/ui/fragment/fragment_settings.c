// Copyright (c) Kuba Szczodrzyński 2025-1-20.

#include "fragment.h"

static view_t *input_player_name		   = NULL;
static view_t *input_public_server_address = NULL;
static view_t *btn_ui_scale				   = NULL;

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	GFX_VIEW_BIND(fragment->views, input_player_name, return false);
	GFX_VIEW_BIND(fragment->views, input_public_server_address, return false);
	GFX_VIEW_BIND(fragment->views, btn_ui_scale, return false);

	// reset fields
	input_player_name->data.input.value[0]			 = '\0';
	input_public_server_address->data.input.value[0] = '\0';

	// load fields from settings
	if (SETTINGS->player_name != NULL)
		strncpy2(input_player_name->data.input.value, SETTINGS->player_name, input_player_name->data.input.max_length);
	if (SETTINGS->public_server_address != NULL)
		strncpy2(
			input_public_server_address->data.input.value,
			SETTINGS->public_server_address,
			input_public_server_address->data.input.max_length
		);

	// if NULL or blank, fill with default values
	if (input_player_name->data.input.value[0] == '\0')
		strcpy(input_player_name->data.input.value, "Player");
	if (input_public_server_address->data.input.value[0] == '\0')
		strcpy(input_public_server_address->data.input.value, "127.0.0.1:5678");

	char buf[24];
	sprintf(buf, "UI Scale: %u", SETTINGS->screen.scale);
	gfx_view_set_text(btn_ui_scale, buf);

	return true;
}

static bool on_btn_ui_scale(view_t *view, SDL_Event *e, ui_t *ui) {
	SETTINGS->screen.scale += 1;
	if (SETTINGS->screen.scale > 3)
		SETTINGS->screen.scale = 1;
	// apply the new scale
	SDL_RenderSetScale(ui->renderer, (float)SETTINGS->screen.scale, (float)SETTINGS->screen.scale);
	// set previous and current sizes and position
	int new_w = SETTINGS->screen.width * SETTINGS->screen.scale;
	int new_h = SETTINGS->screen.height * SETTINGS->screen.scale;
	int x, y, w, h;
	SDL_GetWindowPosition(ui->window, &x, &y);
	SDL_GetWindowSize(ui->window, &w, &h);
	// apply new window size, move the window so it's still in the same position
	SDL_SetWindowSize(ui->window, new_w, new_h);
	SDL_SetWindowPosition(ui->window, x + (w - new_w) / 2, y + (h - new_h) / 2);
	// update the button label
	char buf[24];
	sprintf(buf, "UI Scale: %u", SETTINGS->screen.scale);
	gfx_view_set_text(btn_ui_scale, buf);
	return true;
}

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	FREE_NULL(SETTINGS->player_name);
	SETTINGS->player_name = strdup(input_player_name->data.input.value);
	FREE_NULL(SETTINGS->public_server_address);
	SETTINGS->public_server_address = strdup(input_public_server_address->data.input.value);
	settings_save();

	ui_state_set(ui, UI_STATE_MAIN);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_ui_scale),
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_settings = {
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
