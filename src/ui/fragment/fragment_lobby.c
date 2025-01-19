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
static view_t *btn_game_rename	 = NULL;
static view_t *btn_game_private	 = NULL;
static view_t *btn_game_public	 = NULL;

static unsigned int selected_player_id = 0;

static void ui_update_game(ui_t *ui);
static void ui_update_players(ui_t *ui);
static void ui_update_player(ui_t *ui, player_t *player);
static void ui_remove_player(ui_t *ui, unsigned int player_id);
static void ui_update_status(ui_t *ui);
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
	GFX_VIEW_BIND(fragment->views, btn_game_rename, goto error);
	GFX_VIEW_BIND(fragment->views, btn_game_private, goto error);
	GFX_VIEW_BIND(fragment->views, btn_game_public, goto error);
	if (!dialog_init(ui, fragment->views))
		goto error;

	// clone the player list row
	players_row = gfx_view_clone(players_list->children, players_list);
	gfx_view_free(players_list->children);
	players_list->children = NULL;

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

	player_t *local_player;
	SDL_WITH_MUTEX(GAME->mutex) {
		// find a locally-controlled player
		DL_SEARCH_SCALAR(GAME->players, local_player, is_local, true);
	}
	if (local_player == NULL) {
		// create a player if not yet added
		pkt_player_new_t pkt = {
			.hdr.type = PKT_PLAYER_NEW,
		};
		strncpy2(pkt.name, SETTINGS->player_name, PLAYER_NAME_LEN);
		net_pkt_send_pipe(GAME->endpoints, (pkt_t *)&pkt);
	} else {
		// the lobby of this game was already shown; recreate the player list
		ui_update_players(ui);
	}

	ui_update_game(ui);
	ui_update_status(ui);
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

	ui->force_layout = true;
}

static void ui_update_players(ui_t *ui) {
	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			ui_update_player(ui, player);
		}
	}
}

static void ui_update_player(ui_t *ui, player_t *player) {
	if (player == NULL)
		// nothing to do!
		return;

	view_t *row = gfx_view_find_by_tag(players_list->children, (void *)(uintptr_t)player->id);
	if (row == NULL) {
		// create a new row if not already shown
		row		 = gfx_view_clone(players_row, players_list);
		row->tag = (void *)(uintptr_t)player->id;
		DL_APPEND(players_list->children, row);
	}

	// find all views
	view_t *row_bg, *row_color, *row_name, *row_status, *row_you_icon, *row_you_text;
	GFX_VIEW_BIND(row, row_bg, return);
	GFX_VIEW_BIND(row, row_color, return);
	GFX_VIEW_BIND(row, row_name, return);
	GFX_VIEW_BIND(row, row_status, return);
	GFX_VIEW_BIND(row, row_you_icon, return);
	GFX_VIEW_BIND(row, row_you_text, return);

	row_bg->is_invisible	  = player->id != selected_player_id;
	row_color->data.rect.fill = player->color;
	gfx_view_set_text(row_name, player->name);

	row_you_icon->is_gone = row_you_text->is_gone = !player->is_local;

	const char *status = NULL;
	unsigned int color = 0xA0A0A0;
	switch (player->state) {
		case PLAYER_IDLE:
			status = "Not Ready";
			break;
		case PLAYER_READY:
			status = "Ready";
			color  = GFX_COLOR_BRIGHT_GREEN;
			break;
		case PLAYER_PLAYING:
			status = "In Game";
			break;
		case PLAYER_CRASHED:
			status = "Crashed!";
			color  = GFX_COLOR_BRIGHT_RED;
			break;
		case PLAYER_FINISHED:
			status = "Finished!";
			color  = GFX_COLOR_BRIGHT_GREEN;
			break;
		case PLAYER_DISCONNECTED:
			status = "Disconnected";
			break;
		case PLAYER_SPECTATING:
			status = "Spectating";
			break;
	}
	gfx_view_set_text(row_status, status);
	row_status->data.text.color = color;

	switch (player->turn_key) {
		case SDL_SCANCODE_RSHIFT:
			status = "Right Shift";
			break;
		case SDL_SCANCODE_LSHIFT:
			status = "Left Shift";
			break;
		case SDL_SCANCODE_RCTRL:
			status = "Right Ctrl";
			break;
		case SDL_SCANCODE_LCTRL:
			status = "Left Ctrl";
			break;
		case SDL_SCANCODE_RALT:
			status = "Right Alt";
			break;
		case SDL_SCANCODE_LALT:
			status = "Left Alt";
			break;
		default:
			break;
	}
	gfx_view_set_text(row_you_text, status);

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

static void ui_update_status(ui_t *ui) {
	int players_count = 0;
	int ready_count	  = 0;
	int local_count	  = 0;
	int in_game_count = 0;
	bool self_ready	  = false;

	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			players_count++;
			if (player->state == PLAYER_READY) {
				ready_count++;
				if (player->is_local)
					self_ready = true;
			} else if (player->state != PLAYER_IDLE && player->state != PLAYER_SPECTATING) {
				in_game_count++;
			}
			if (player->is_local)
				local_count++;
		}
	}

	bool lock_controls = false;
	if (in_game_count != 0 || GAME->state != GAME_IDLE) {
		gfx_view_set_text(text_status, "Players are currently\nin game...");
		btn_ready->is_disabled = true;
		gfx_view_set_text(btn_ready, "READY");
		lock_controls = true;
	} else if (self_ready) {
		char buf[64];
		if (players_count - ready_count)
			snprintf(buf, sizeof(buf), "Waiting for\n%d player(s)...", players_count - ready_count);
		else
			strcpy(buf, "Starting\nthe game...");
		gfx_view_set_text(text_status, buf);
		btn_ready->is_disabled = true;
		gfx_view_set_text(btn_ready, "READY");
		lock_controls = players_count == ready_count;
	} else if (players_count < 2) {
		gfx_view_set_text(text_status, "There's nobody\nhere...");
		btn_ready->is_disabled = false;
		gfx_view_set_text(btn_ready, "Play Alone");
	} else if (ready_count < players_count - local_count) {
		gfx_view_set_text(text_status, "Are you\nready?");
		btn_ready->is_disabled = false;
		gfx_view_set_text(btn_ready, "READY");
	} else {
		gfx_view_set_text(text_status, "Everyone is\nready!");
		btn_ready->is_disabled = false;
		gfx_view_set_text(btn_ready, "START");
	}

	slider_speed->is_disabled	  = lock_controls;
	btn_game_rename->is_disabled  = lock_controls;
	btn_game_private->is_disabled = !GAME->is_public || lock_controls;
	btn_game_public->is_disabled  = GAME->is_public || lock_controls;
	btn_player_ban->is_disabled	  = btn_player_ban->is_disabled || lock_controls;

	ui->force_layout = true;
}

static void ui_dialog_cb(ui_t *ui, int dialog_id, const char *value) {
	player_t *player = NULL;

	switch (dialog_id) {
		case DIALOG_GAME_RENAME:
			strncpy2(GAME->name, value, GAME_NAME_LEN);
			game_request_send_update(GAME, true, 0);
			ui_update_game(ui);
			ui_update_status(ui);
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
			ui_update_player(ui, player);
			ui_update_status(ui);
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
	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if (!player->is_local || player->state != PLAYER_IDLE)
				continue;
			player->state = PLAYER_READY;
			game_request_send_update(GAME, false, player->id);
			ui_update_player(ui, player);
			ui_update_status(ui);
		}
	}
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
	btn_player_ban->is_disabled	   = player->is_local || GAME->state != GAME_IDLE;
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
	ui_update_status(ui);
	return false;
}

static bool on_btn_game_public(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->is_public = true;
	game_request_send_update(GAME, true, 0);
	ui_update_game(ui);
	ui_update_status(ui);
	return false;
}

static bool on_speed_change(view_t *view, SDL_Event *e, ui_t *ui) {
	GAME->speed = view->data.slider.value;
	game_request_send_update(GAME, true, 0);
	ui_update_game(ui);
	ui_update_status(ui);
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
					ui_update_status(ui);
					break;
				case PKT_PLAYER_DATA:
					ui_update_player(ui, game_get_player_by_id(GAME, pkt->player_data.id));
					ui_update_status(ui);
					break;
				case PKT_PLAYER_LEAVE:
					ui_remove_player(ui, pkt->player_leave.id);
					ui_update_status(ui);
					break;
				case PKT_GAME_START:
					ui_state_set(ui, UI_STATE_MATCH);
					break;
				case PKT_GAME_STOP:
					// game stopped - we're most likely spectating
					ui_update_players(ui);
					ui_update_status(ui);
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
