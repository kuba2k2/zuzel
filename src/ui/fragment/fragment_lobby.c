// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "fragment.h"

#define GAME (ui->game)

typedef enum {
	DIALOG_GAME_RENAME,
	DIALOG_GAME_QUIT,
	DIALOG_PLAYER_RENAME,
	DIALOG_PLAYER_BAN,
} dialog_id_t;

static view_t *text_name		 = NULL;
static view_t *text_key			 = NULL;
static view_t *slider_speed		 = NULL;
static view_t *text_status		 = NULL;
static view_t *btn_ready		 = NULL;
static view_t *players_list		 = NULL;
static view_t *players_row		 = NULL;
static view_t *btn_player_rename = NULL;
static view_t *btn_player_ban	 = NULL;
static view_t *btn_game_private	 = NULL;
static view_t *btn_game_public	 = NULL;

static unsigned int selected_player_id = 0;

static void ui_update_game(ui_t *ui);
static void ui_update_player(ui_t *ui, unsigned int player_id);
static void ui_remove_player(ui_t *ui, unsigned int player_id);
static void on_quit(ui_t *ui);
static void on_error(ui_t *ui);

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
	if (!dialog_init(ui, fragment->views))
		goto error;

	// clone the player list row
	players_row = gfx_view_clone(players_list->children, players_list);
	gfx_view_free(players_list->children);
	players_list->children		   = NULL;
	btn_player_rename->is_disabled = true;
	btn_player_ban->is_disabled	   = true;
	btn_game_private->is_gone	   = GAME->is_local;
	btn_game_public->is_gone	   = GAME->is_local;
	selected_player_id			   = 0;

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

	view_t *row = gfx_view_find_by_tag(players_list->children, (void *)(uintptr_t)player_id);
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

	row_bg->is_invisible	  = player->id != selected_player_id;
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

static void ui_remove_player(ui_t *ui, unsigned int player_id) {
	view_t *row = gfx_view_find_by_tag(players_list->children, (void *)(uintptr_t)player_id);
	if (row == NULL)
		return;
	DL_DELETE(players_list->children, row);
	row->next = NULL;
	row->prev = row;
	gfx_view_free(row);
	ui->force_layout = true;
}

static void ui_dialog_cb(ui_t *ui, int dialog_id, const char *value) {
	player_t *player = NULL;

	switch (dialog_id) {
		case DIALOG_GAME_RENAME:
			strncpy2(GAME->name, value, GAME_NAME_LEN);
			game_request_send_update(GAME, true, 0);
			ui_update_game(ui);
			break;

		case DIALOG_GAME_QUIT:
			on_quit(ui);
			break;

		case DIALOG_PLAYER_RENAME:
			player = game_get_player_by_id(GAME, selected_player_id);
			if (player == NULL)
				break;
			strncpy2(player->name, value, PLAYER_NAME_LEN);
			game_request_send_update(GAME, false, selected_player_id);
			// unselect the modified player
			selected_player_id			   = 0;
			btn_player_rename->is_disabled = true;
			btn_player_ban->is_disabled	   = true;
			// update the UI
			ui_update_player(ui, player->id);
			break;

		case DIALOG_PLAYER_BAN: {
			// send a ban request to the server
			pkt_player_leave_t pkt = {
				.hdr.type = PKT_PLAYER_LEAVE,
				.id		  = selected_player_id,
			};
			net_pkt_send_pipe(GAME->endpoints, (pkt_t *)&pkt);
			break;
		}

		default:
			break;
	}
	dialog_hide(ui);
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
	selected_player_id = (uintptr_t)view->parent->tag;
	player_t *player   = game_get_player_by_id(GAME, selected_player_id);
	if (player == NULL) {
		btn_player_rename->is_disabled = true;
		btn_player_ban->is_disabled	   = true;
		return false;
	}
	btn_player_rename->is_disabled = !player->is_local;
	btn_player_ban->is_disabled	   = player->is_local;
	return true;
}

static bool on_btn_player_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	player_t *player = game_get_player_by_id(GAME, selected_player_id);
	if (player == NULL)
		return false;
	dialog_show_edit(ui, DIALOG_PLAYER_RENAME, ui_dialog_cb, "Rename Player", player->name, PLAYER_NAME_LEN);
	return true;
}

static bool on_btn_player_ban(view_t *view, SDL_Event *e, ui_t *ui) {
	player_t *player = game_get_player_by_id(GAME, selected_player_id);
	if (player == NULL)
		return false;
	char buf[64];
	snprintf(buf, sizeof(buf), "Ban %s?", player->name);
	dialog_show_prompt(ui, DIALOG_PLAYER_BAN, ui_dialog_cb, buf, "They will be able to join again using the game key.");
	return true;
}

static bool on_btn_game_rename(view_t *view, SDL_Event *e, ui_t *ui) {
	dialog_show_edit(ui, DIALOG_GAME_RENAME, ui_dialog_cb, "Rename Game", GAME->name, GAME_NAME_LEN);
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
	const char *message = "Other players will continue playing.";
	if (ui->connection.type == UI_CONNECT_NEW_LOCAL) {
		message = "This is a local game:\nother players will be disconnected!";
	}
	dialog_show_prompt(ui, DIALOG_GAME_QUIT, ui_dialog_cb, "Really Quit?", message);
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
				case PKT_PLAYER_LEAVE:
					ui_remove_player(ui, pkt->player_leave.id);
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
