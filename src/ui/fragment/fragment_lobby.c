// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "fragment.h"

#define GAME (ui->game)

static view_t *text_name			 = NULL;
static view_t *text_key				 = NULL;
static view_t *slider_speed			 = NULL;
static view_t *text_status			 = NULL;
static view_t *btn_ready			 = NULL;
static view_t *players_list			 = NULL;
static view_t *players_row			 = NULL;
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

static player_t *selected_player = NULL;

static void ui_update_game(ui_t *ui);
static void ui_update_player(ui_t *ui, unsigned int player_id);
static void on_quit(ui_t *ui);
static void on_error(ui_t *ui);
static void ui_hide_dialog(ui_t *ui);
static void ui_show_dialog_edit(ui_t *ui, const char *title, const char *value, int max_length);
static void ui_show_dialog_prompt(ui_t *ui, const char *title, const char *message);

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
	GFX_VIEW_BIND(fragment->views, players_list, goto error);
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

	// clone the player list row
	players_row = gfx_view_clone(players_list->children, players_list);
	gfx_view_free(players_list->children);
	players_list->children		   = NULL;
	selected_player				   = NULL;
	btn_player_rename->is_disabled = true;
	btn_player_ban->is_disabled	   = true;

	btn_game_private->is_gone = GAME->is_local;
	btn_game_public->is_gone  = GAME->is_local;
	ui_hide_dialog(ui);

	switch (ui->connection.type) {
		case UI_CONNECT_NEW_PUBLIC:
		case UI_CONNECT_NEW_PRIVATE:
			// the game was just created, set the player's customized data and send an update
			game_set_default_player_options(GAME);
			game_request_send_update(GAME, true, 0);
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

	SDL_WITH_MUTEX(GAME->mutex) {
		// find a locally-controlled player
		player_t *local_player;
		DL_SEARCH_SCALAR(GAME->players, local_player, is_local, true);
		if (local_player == NULL) {
			// create a player if not yet added
			pkt_player_new_t pkt = {
				.hdr.type = PKT_PLAYER_NEW,
			};
			strncpy2(pkt.name, SETTINGS->player_name, PLAYER_NAME_LEN);
			net_pkt_send_pipe(GAME->endpoints, (pkt_t *)&pkt);
		}
	}

	ui_update_game(ui);
	return true;

error:
	on_error(ui);
	return false;
}

static bool on_hide(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	if (players_list == NULL || players_row == NULL)
		return false;
	// free the current list box contents
	gfx_view_free(players_list->children);
	players_list->children = NULL;
	// put the cloned row back into the list
	DL_APPEND(players_list->children, players_row);
	return true;
}

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

static void ui_update_player(ui_t *ui, unsigned int player_id) {
	player_t *player = game_get_player_by_id(GAME, player_id);
	if (player == NULL)
		// nothing to do!
		return;

	view_t *row = gfx_view_find_by_tag(players_list, (void *)(uintptr_t)player_id);
	if (row == NULL) {
		// create a new row if not already shown
		row		 = gfx_view_clone(players_row, players_list);
		row->tag = (void *)(uintptr_t)player_id;
		DL_APPEND(players_list->children, row);
	}

	// find all views
	view_t *row_bg, *row_color, *row_name, *row_status;
	GFX_VIEW_BIND(row, row_bg, return);
	GFX_VIEW_BIND(row, row_color, return);
	GFX_VIEW_BIND(row, row_name, return);
	GFX_VIEW_BIND(row, row_status, return);

	row_bg->is_invisible	  = player != selected_player;
	row_color->data.rect.fill = player->color;
	gfx_view_set_text(row_name, player->name);

	const char *status = NULL;
	switch (player->state) {
		case PLAYER_IDLE:
			status = "Not Ready";
			break;
		case PLAYER_READY:
			status = "Ready";
			break;
		case PLAYER_PLAYING:
			status = "In Game";
			break;
		case PLAYER_CRASHED:
			status = "Crashed!";
			break;
		case PLAYER_FINISHED:
			status = "Finished!";
			break;
		case PLAYER_DISCONNECTED:
			status = "Disconnected";
			break;
	}
	gfx_view_set_text(row_status, status);

	ui->force_layout = true;
}

static bool on_btn_ready(view_t *view, SDL_Event *e, ui_t *ui) {
	return false;
}

static bool on_btn_row(view_t *view, SDL_Event *e, ui_t *ui) {
	view_t *row;
	DL_FOREACH(players_list->children, row) {
		view_t *row_bg		 = row->children;
		row_bg->is_invisible = view->parent != row;
	}
	unsigned int player_id = (uintptr_t)view->parent->tag;
	selected_player		   = game_get_player_by_id(GAME, player_id);
	if (selected_player == NULL)
		return false;
	btn_player_rename->is_disabled = !selected_player->is_local;
	btn_player_ban->is_disabled	   = selected_player->is_local;
	return true;
}

static bool on_btn_player_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	if (selected_player == NULL)
		return false;
	in_game_rename	 = false;
	in_player_rename = true;
	ui_show_dialog_edit(ui, "Rename Player", selected_player->name, PLAYER_NAME_LEN);
	return true;
}

static bool on_btn_player_ban(view_t *view, SDL_Event *e, ui_t *ui) {
	if (selected_player == NULL)
		return false;
	in_quit_confirm = false;
	in_ban_confirm	= true;
	char buf[64];
	snprintf(buf, sizeof(buf), "Ban %s?", selected_player->name);
	ui_show_dialog_prompt(ui, buf, "They will be able to join again using the game key.");
	return true;
}

static bool on_btn_game_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	in_game_rename	 = true;
	in_player_rename = false;
	ui_show_dialog_edit(ui, "Rename Game", GAME->name, GAME_NAME_LEN);
	return false;
}

static bool on_btn_game_private(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->is_public = false;
	game_request_send_update(GAME, true, 0);
	ui_update_game(ui);
	return false;
}

static bool on_btn_game_public(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->is_public = true;
	game_request_send_update(GAME, true, 0);
	ui_update_game(ui);
	return false;
}

static bool on_speed_change(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->speed = view->data.slider.value;
	game_request_send_update(GAME, true, 0);
	ui_update_game(ui);
	return false;
}

static bool on_btn_quit(view_t *view, SDL_Event *e, ui_t *ui) {
	if (game_get_player_count(GAME) == 1) {
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
	ui_show_dialog_prompt(ui, "Really Quit?", message);
	return true;
}

static void ui_hide_dialog(ui_t *ui) {
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

static void ui_show_dialog_edit(ui_t *ui, const char *title, const char *value, int max_length) {
	dialog_bg->is_gone	   = false;
	dialog_edit->is_gone   = false;
	dialog_prompt->is_gone = true;
	gfx_view_set_text(dialog_edit_title, title);
	// set input value
	free(dialog_edit_input->data.input.value);
	MALLOC(dialog_edit_input->data.input.value, max_length + 1, return);
	strncpy2(dialog_edit_input->data.input.value, value, max_length);
	dialog_edit_input->data.input.max_length = max_length;
	dialog_edit_input->data.input.pos		 = strlen(dialog_edit_input->data.input.value);
	ui->force_layout						 = true;
}

static void ui_show_dialog_prompt(ui_t *ui, const char *title, const char *message) {
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
		strncpy2(GAME->name, dialog_edit_input->data.input.value, GAME_NAME_LEN);
		game_request_send_update(GAME, true, 0);
		ui_update_game(ui);
		in_game_rename = false;
	}
	if (in_player_rename && selected_player != NULL) {
		strncpy2(selected_player->name, dialog_edit_input->data.input.value, PLAYER_NAME_LEN);
		game_request_send_update(GAME, false, selected_player->id);
		ui_update_player(ui, selected_player->id);
		in_player_rename = false;
	}
	ui_hide_dialog(ui);
	return true;
}

static bool on_dialog_prompt_yes(view_t *view, SDL_Event *e, ui_t *ui) {
	if (in_quit_confirm) {
		on_quit(ui);
	}
	ui_hide_dialog(ui);
	return true;
}

static bool on_dialog_prompt_no(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_hide_dialog(ui);
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
	pkt_t *pkt = NULL;
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
			pkt = e->user.data1;
			if (pkt == NULL)
				return false;
			switch (pkt->hdr.type) {
				case PKT_GAME_DATA:
					ui_update_game(ui);
					break;
				case PKT_PLAYER_DATA:
					ui_update_player(ui, pkt->player_data.id);
					break;
				default:
					return false;
			}
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
	.on_hide		  = on_hide,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
