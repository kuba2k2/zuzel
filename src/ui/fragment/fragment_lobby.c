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

static bool in_game_rename	 = false;
static bool in_player_rename = false;
static bool in_quit_confirm	 = false;
static bool in_ban_confirm	 = false;

static void on_quit(ui_t *ui);
static void on_error(ui_t *ui);
static void hide_dialog(ui_t *ui);
static void show_dialog_edit(ui_t *ui, const char *title, const char *value, int max_length);
static void show_dialog_prompt(ui_t *ui, const char *title, const char *message);

static void ui_update_game(ui_t *ui) {
	char buf[64];

	gfx_view_set_text(text_name, GAME->name);
	snprintf(buf, sizeof(buf), "Speed: %d", GAME->speed);
	gfx_view_set_text(slider_speed, buf);
	slider_speed->data.slider.value = (int)GAME->speed;
	btn_game_private->is_disabled	= !GAME->is_public;
	btn_game_public->is_disabled	= GAME->is_public;

	ui->force_layout = true;
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

	btn_game_private->is_gone = GAME->is_local;
	btn_game_public->is_gone  = GAME->is_local;
	hide_dialog(ui);

	switch (ui->connection.type) {
		case UI_CONNECT_NEW_PUBLIC:
		case UI_CONNECT_NEW_PRIVATE:
			// the game was just created, set the player's customized data and send an update
			game_set_default_player_options(GAME);
			game_request_send_game_data(GAME);
			// show the game key
			gfx_view_set_text(text_key, GAME->key);
			break;

		case UI_CONNECT_NEW_LOCAL:
			// show the local IP address(es)
			gfx_view_set_text(text_key, GAME->local_ips);
			break;

		case UI_CONNECT_JOIN_BROWSE:
		case UI_CONNECT_JOIN_KEY:
			// show the game key
			gfx_view_set_text(text_key, GAME->key);
			break;

		case UI_CONNECT_JOIN_ADDRESS:
			// show the connection address
			gfx_view_set_text(text_key, ui->connection.address);
			break;
	}

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
	in_game_rename	 = true;
	in_player_rename = false;
	show_dialog_edit(ui, "Rename Game", GAME->name, GAME_NAME_LEN);
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

static bool on_btn_quit(view_t *view, SDL_Event *e, ui_t *ui) {
	if (GAME->player_count == 1) {
		// don't ask if we're the only player
		on_quit(ui);
		return true;
	}
	in_quit_confirm		= true;
	in_ban_confirm		= false;
	const char *message = "Other players will continue playing.";
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		message = "This is a local game:\nother players will be disconnected!";
	}
	show_dialog_prompt(ui, "Really Quit?", message);
	return true;
}

static void hide_dialog(ui_t *ui) {
	dialog_bg->is_gone	   = true;
	dialog_edit->is_gone   = true;
	dialog_prompt->is_gone = true;
	in_game_rename		   = false;
	in_player_rename	   = false;
	in_quit_confirm		   = false;
	in_ban_confirm		   = false;
	// need to manually reset bounding boxes of all child views
	view_t *view = dialog_bg;
	while (view != NULL) {
		memset(&view->rect, 0, sizeof(view->rect));
		view = gfx_view_find_next(view);
	}
	ui->force_layout = true;
}

static void show_dialog_edit(ui_t *ui, const char *title, const char *value, int max_length) {
	dialog_bg->is_gone	   = false;
	dialog_edit->is_gone   = false;
	dialog_prompt->is_gone = true;
	gfx_view_set_text(dialog_edit_title, title);
	// set input value
	free(dialog_edit_input->data.input.value);
	MALLOC(dialog_edit_input->data.input.value, max_length + 2, return);
	strncpy(dialog_edit_input->data.input.value, value, max_length);
	dialog_edit_input->data.input.max_length = max_length;
	dialog_edit_input->data.input.pos		 = strlen(dialog_edit_input->data.input.value);
	ui->force_layout						 = true;
}

static void show_dialog_prompt(ui_t *ui, const char *title, const char *message) {
	dialog_bg->is_gone	   = false;
	dialog_edit->is_gone   = true;
	dialog_prompt->is_gone = false;
	gfx_view_set_text(dialog_prompt_title, title);
	gfx_view_set_text(dialog_prompt_message, message);
	ui->force_layout = true;
}

static bool on_dialog_edit_input(view_t *view, SDL_Event *e, ui_t *ui) {
	dialog_edit_ok->is_disabled = strlen(view->data.input.value) == 0;
	return true;
}

static bool on_dialog_edit_ok(view_t *view, SDL_Event *e, ui_t *ui) {
	if (in_game_rename) {
		strncpy(GAME->name, dialog_edit_input->data.input.value, GAME_NAME_LEN);
		game_request_send_game_data(GAME);
		ui_update_game(ui);
		in_game_rename = false;
	}
	hide_dialog(ui);
	return true;
}

static bool on_dialog_prompt_yes(view_t *view, SDL_Event *e, ui_t *ui) {
	if (in_quit_confirm) {
		on_quit(ui);
	}
	hide_dialog(ui);
	return true;
}

static bool on_dialog_prompt_no(view_t *view, SDL_Event *e, ui_t *ui) {
	hide_dialog(ui);
	return true;
}

static void on_quit(ui_t *ui) {
	game_stop(GAME);
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
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
