// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "fragment.h"

#define GAME (ui->game)

static view_t *text_name			 = NULL;
static view_t *text_key				 = NULL;
static view_t *slider_speed			 = NULL;
static view_t *text_status			 = NULL;
static view_t *btn_ready			 = NULL;
static view_t *btn_player_rename	 = NULL;
static view_t *btn_player_ban		 = NULL;
static view_t *btn_game_private		 = NULL;
static view_t *btn_game_public		 = NULL;
static view_t *dialog_bg			 = NULL;
static view_t *dialog_edit			 = NULL;
static view_t *dialog_edit_title	 = NULL;
static view_t *dialog_edit_input	 = NULL;
static view_t *dialog_edit_ok		 = NULL;
static view_t *dialog_prompt		 = NULL;
static view_t *dialog_prompt_title	 = NULL;
static view_t *dialog_prompt_message = NULL;
static view_t *dialog_prompt_yes	 = NULL;
static view_t *dialog_prompt_no		 = NULL;

static void on_error(ui_t *ui);

static void ui_update_game(ui_t *ui) {
	char buf[64];

	gfx_view_set_text(text_name, GAME->name);
	gfx_view_set_text(text_key, GAME->key);
	snprintf(buf, sizeof(buf), "Speed: %d", GAME->speed);
	gfx_view_set_text(slider_speed, buf);
	slider_speed->data.slider.value = (int)GAME->speed;
	btn_game_private->is_disabled	= !GAME->is_public;
	btn_game_public->is_disabled	= GAME->is_public;
}

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	if (GAME == NULL) {
		LT_E("Game is not provided");
		goto error;
	}

	GFX_VIEW_BIND(fragment->views, text_name, goto error);
	GFX_VIEW_BIND(fragment->views, text_key, goto error);
	GFX_VIEW_BIND(fragment->views, slider_speed, goto error);
	GFX_VIEW_BIND(fragment->views, text_status, goto error);
	GFX_VIEW_BIND(fragment->views, btn_ready, goto error);
	GFX_VIEW_BIND(fragment->views, btn_player_rename, goto error);
	GFX_VIEW_BIND(fragment->views, btn_player_ban, goto error);
	GFX_VIEW_BIND(fragment->views, btn_game_private, goto error);
	GFX_VIEW_BIND(fragment->views, btn_game_public, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_bg, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_edit, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_edit_title, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_edit_input, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_edit_ok, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_prompt, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_prompt_title, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_prompt_message, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_prompt_yes, goto error);
	GFX_VIEW_BIND(fragment->views, dialog_prompt_no, goto error);

	ui_update_game(ui);
	return true;

error:
	on_error(ui);
	return false;
}

static bool on_btn_ready(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_row(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_player_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_player_ban(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_game_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_game_private(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->is_public = false;
	game_request_send_game_data(GAME);
	ui_update_game(ui);
	return false;
}

static bool on_btn_game_public(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->is_public = true;
	game_request_send_game_data(GAME);
	ui_update_game(ui);
	return false;
}

static bool on_speed_change(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->speed = view->data.slider.value;
	game_request_send_game_data(GAME);
	ui_update_game(ui);
	return false;
}

static bool on_dialog_edit_input(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_dialog_edit_ok(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_dialog_prompt_yes(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_dialog_prompt_no(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_quit(view_t *view, SDL_Event *e, ui_t *ui) {
	game_stop(GAME);
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
	return true;
}

static void on_error(ui_t *ui) {
	game_stop(GAME);
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
	ui_state_set(ui, UI_STATE_ERROR);
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	switch (e->type) {
		case SDL_USEREVENT_GAME:
			if (e->user.data1 != NULL)
				LT_E("Unexpected new game packet received");
			else
				LT_E("Game stopped unexpectedly - perhaps server connection was lost?");
			GAME = NULL;
			on_error(ui);
			return true;

		case SDL_USEREVENT_PACKET:
			LT_I("Received packet");
			ui_update_game(ui);
			return true;
	}
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_ready),
	GFX_VIEW_ON_EVENT(on_btn_row),
	GFX_VIEW_ON_EVENT(on_btn_player_rename),
	GFX_VIEW_ON_EVENT(on_btn_player_ban),
	GFX_VIEW_ON_EVENT(on_btn_game_rename),
	GFX_VIEW_ON_EVENT(on_btn_game_private),
	GFX_VIEW_ON_EVENT(on_btn_game_public),
	GFX_VIEW_ON_EVENT(on_speed_change),
	GFX_VIEW_ON_EVENT(on_dialog_edit_input),
	GFX_VIEW_ON_EVENT(on_dialog_edit_ok),
	GFX_VIEW_ON_EVENT(on_dialog_prompt_yes),
	GFX_VIEW_ON_EVENT(on_dialog_prompt_no),
	GFX_VIEW_ON_EVENT(on_btn_quit),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_lobby = {
	.on_show		  = on_show,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
