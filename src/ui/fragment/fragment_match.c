// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "fragment.h"

#include "ui/match/match_gfx.h"
#include "ui/match/match_input.h"

#define GAME (ui->game)

static view_t *canvas		 = NULL;
static view_t *text_top		 = NULL;
static view_t *match_info	 = NULL;
static view_t *players_title = NULL;
static view_t *players_list	 = NULL;
static view_t *player_state	 = NULL;
static view_t *ready_info	 = NULL;

static bool first_draw = false;

static void match_update_redraw_all(ui_t *ui);
static void match_update_state(ui_t *ui);
static void match_update_player_state(ui_t *ui);
static void match_update_players(ui_t *ui, bool redraw);
static void on_draw(SDL_Renderer *renderer, view_t *canvas);
static void on_error(ui_t *ui);
static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e);

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	if (GAME == NULL) {
		LT_E("Game is not provided");
		goto error;
	}

	GFX_VIEW_BIND(fragment->views, canvas, goto error);
	GFX_VIEW_BIND(fragment->views, text_top, goto error);
	GFX_VIEW_BIND(fragment->views, match_info, goto error);
	GFX_VIEW_BIND(fragment->views, players_title, goto error);
	GFX_VIEW_BIND(fragment->views, players_list, goto error);
	GFX_VIEW_BIND(fragment->views, player_state, goto error);
	GFX_VIEW_BIND(fragment->views, ready_info, goto error);

	first_draw	 = true;
	canvas->draw = on_draw;

	return true;

error:
	on_error(ui);
	return false;
}

static void match_update_redraw_all(ui_t *ui) {
	SDL_SetRenderTarget(ui->renderer, canvas->data.canvas.texture);
	match_gfx_board_draw(ui->renderer);
	match_update_state(ui);
	match_update_player_state(ui);

	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if (player->state != PLAYER_PLAYING && player->state != PLAYER_CRASHED && player->state != PLAYER_FINISHED)
				continue;
			SDL_WITH_MUTEX(player->mutex) {
				SDL_SetRenderTarget(ui->renderer, player->texture);
				match_gfx_player_draw(ui->renderer, player);
			}
		}
	}
	SDL_SetRenderTarget(ui->renderer, NULL);
}

static void match_update_state(ui_t *ui) {
	// draw gates if game is starting
	SDL_SetRenderTarget(ui->renderer, canvas->data.canvas.texture);
	match_gfx_gates_draw(ui->renderer, GAME->state < GAME_PLAYING);
	// update texts
	text_top->is_gone			= true;
	match_info->is_gone			= true;
	players_title->is_gone		= true;
	players_list->is_gone		= true;
	match_info->data.text.color = GFX_COLOR_BRIGHT_YELLOW;
	char buf[32];
	switch (GAME->state) {
		case GAME_IDLE:
		case GAME_STARTING:
			match_info->is_gone			= false;
			match_info->data.text.color = GFX_COLOR_BRIGHT_BLACK;
			gfx_view_set_text(match_info, "Starting...");
			break;
		case GAME_COUNTING:
			match_info->is_gone = false;
			sprintf(buf, "Starting in %d...", GAME->start_in);
			gfx_view_set_text(match_info, buf);
			break;
		case GAME_PLAYING:
			match_info->is_gone = false;
			sprintf(buf, "Round %d. Lap: %d", GAME->round, GAME->lap);
			gfx_view_set_text(match_info, buf);
			break;
		case GAME_FINISHED:
			players_title->is_gone = false;
			players_list->is_gone  = false;
			break;
	}
	ui->force_layout = true;
}

static void match_update_player_state(ui_t *ui) {
	// update texts
	player_state->is_gone = true;
	ready_info->is_gone	  = true;

	player_t *local_player = NULL;
	int local_state		   = 0;

	int players_count	 = 0;
	int players_in_round = 0;
	int players_ready	 = 0;
	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if ((player->state & PLAYER_IN_GAME_MASK) == 0)
				continue;
			players_count++;
			if (player->is_in_round)
				players_in_round++;
			if (player->state == PLAYER_READY)
				players_ready++;
			if (player->is_local) {
				local_player = player;
				if (local_state == 0)
					local_state = player->state;
				else if (local_state != player->state)
					local_state = -1;
			}
		}
	}
	if (local_player == NULL)
		return;
	if (local_player->state == PLAYER_PLAYING || local_state == -1)
		// no texts if we're still alive, or the local player states are not all the same
		return;

	if (local_player->state == PLAYER_CRASHED) {
		gfx_view_set_text(player_state, "You crashed!");
		player_state->data.text.color = GFX_COLOR_BRIGHT_RED;
		player_state->is_gone		  = false;
	} else if (local_player->state == PLAYER_FINISHED) {
		gfx_view_set_text(player_state, "You finished!");
		player_state->data.text.color = GFX_COLOR_BRIGHT_GREEN;
		player_state->is_gone		  = false;
	}

	char buf[64];
	if (local_player->state != PLAYER_READY) {
		if (GAME->round > GAME->rounds || (GAME->round == GAME->rounds && players_in_round))
			gfx_view_set_text(ready_info, "Game finished. To go back,\npress <ENTER>");
		else
			gfx_view_set_text(ready_info, "Ready for the next round?\nPress <ENTER>");
		ready_info->data.text.color = GFX_COLOR_BRIGHT_GREEN;
		ready_info->is_gone			= false;
	} else if (players_in_round) {
		sprintf(buf, "%d player(s)\nstill playing.", players_in_round);
		gfx_view_set_text(ready_info, buf);
		ready_info->data.text.color = GFX_COLOR_BRIGHT_BLACK;
		ready_info->is_gone			= false;
	} else if (players_ready != players_count) {
		if (GAME->round > GAME->rounds)
			sprintf(buf, "Waiting for\n%d player(s)...", players_count - players_ready);
		else
			sprintf(buf, "%d player(s)\nnot ready yet!", players_count - players_ready);
		gfx_view_set_text(ready_info, buf);
		ready_info->data.text.color = GFX_COLOR_WHITE;
		ready_info->is_gone			= false;
	}

	ui->force_layout = true;
}

static void match_update_players(ui_t *ui, bool redraw) {
	SDL_WITH_MUTEX(GAME->mutex) {
		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if (!player->is_in_round)
				continue;
			SDL_WITH_MUTEX(player->mutex) {
				SDL_SetRenderTarget(ui->renderer, player->texture);
				if (redraw)
					match_gfx_player_draw(ui->renderer, player);
				else
					match_gfx_player_draw_step(ui->renderer, player);
			}
		}
	}
	SDL_SetRenderTarget(ui->renderer, NULL);
}

// on_draw() will be called right after on_show()
static void on_draw(SDL_Renderer *renderer, view_t *view) {
	ui_t *ui = view->event.param;

	// first draw: recreate all used textures
	if (first_draw) {
		first_draw = false;
		SDL_DestroyTexture(canvas->data.canvas.texture);
		canvas->data.canvas.texture =
			SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
		SDL_SetTextureBlendMode(canvas->data.canvas.texture, SDL_BLENDMODE_BLEND);

		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if ((player->state & PLAYER_IN_GAME_MASK) == 0)
				continue;
			SDL_DestroyTexture(player->texture);
			player->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
			SDL_SetTextureBlendMode(player->texture, SDL_BLENDMODE_BLEND);
		}

		// make the UI redraw everything
		match_update_redraw_all(ui);
	}

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, canvas->data.canvas.texture, NULL, &view->rect);
	player_t *player;
	DL_FOREACH(GAME->players, player) {
		if (player->texture == NULL)
			continue;
		SDL_RenderCopy(renderer, player->texture, NULL, &view->rect);
	}
}

static void on_error(ui_t *ui) {
	game_stop(GAME);
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
	ui_state_set(ui, UI_STATE_ERROR);
}

static void on_quit(ui_t *ui) {
	ui_state_prev(ui);
}

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	pkt_t *pkt = NULL;
	switch (e->type) {
		case SDL_KEYDOWN:
			return match_input_process_key_event(ui, e->key.keysym.scancode, true, match_update_player_state, on_quit);
		case SDL_KEYUP:
			return match_input_process_key_event(ui, e->key.keysym.scancode, false, match_update_player_state, on_quit);

		case SDL_MOUSEBUTTONDOWN:
			return match_input_process_mouse_event(
				ui,
				e->button.x,
				e->button.y,
				true,
				match_update_player_state,
				on_quit
			);
		case SDL_MOUSEBUTTONUP:
			return match_input_process_mouse_event(
				ui,
				e->button.x,
				e->button.y,
				false,
				match_update_player_state,
				on_quit
			);

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
				case PKT_GAME_STOP:
					ui_state_prev(ui);
					break;
				default:
					return false;
			}
			return true;

		case SDL_USEREVENT_MATCH:
			switch (e->user.code) {
				case MATCH_UPDATE_REDRAW_ALL:
					match_update_redraw_all(ui);
					return true;
				case MATCH_UPDATE_STATE:
					match_update_state(ui);
					match_update_player_state(ui);
					match_update_players(ui, false);
					return true;
				case MATCH_UPDATE_STEP_PLAYERS:
					match_update_players(ui, false);
					return true;
				case MATCH_UPDATE_REDRAW_PLAYERS:
					match_update_player_state(ui);
					match_update_players(ui, true);
					return true;
			}
			return false;
	}
	return false;
}

fragment_t fragment_match = {
	.on_show  = on_show,
	.on_event = on_event,
};
