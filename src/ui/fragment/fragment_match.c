// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "fragment.h"

#include "ui/match/match_gfx.h"

#define GAME (ui->game)

static view_t *canvas		 = NULL;
static view_t *text_top		 = NULL;
static view_t *text_info	 = NULL;
static view_t *players_title = NULL;
static view_t *players_list	 = NULL;

static bool first_draw = false;

static void match_update_redraw_all(ui_t *ui);
static void match_update_state(ui_t *ui);
static void match_update_step_players(ui_t *ui);
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
	GFX_VIEW_BIND(fragment->views, text_info, goto error);
	GFX_VIEW_BIND(fragment->views, players_title, goto error);
	GFX_VIEW_BIND(fragment->views, players_list, goto error);

	first_draw	 = true;
	canvas->draw = on_draw;

	return true;

error:
	on_error(ui);
	return false;
}

static void match_update_redraw_all(ui_t *ui) {
	SDL_SetRenderTarget(ui->renderer, canvas->data.canvas.texture);
	match_board_draw(ui->renderer);
	match_update_state(ui);

	player_t *player = NULL;
	DL_FOREACH(GAME->players, player) {
		SDL_SetRenderTarget(ui->renderer, player->texture);
		match_player_draw(ui->renderer, player);
	}
	SDL_SetRenderTarget(ui->renderer, NULL);
}

static void match_update_state(ui_t *ui) {
	// draw gates if game is starting
	match_board_draw_gates(ui->renderer, GAME->state < GAME_PLAYING);
	// update texts
	text_top->is_gone	   = true;
	text_info->is_gone	   = true;
	players_title->is_gone = true;
	players_list->is_gone  = true;
	char buf[32];
	switch (GAME->state) {
		case GAME_IDLE:
		case GAME_STARTING:
			break;
		case GAME_COUNTING:
			text_info->is_gone = false;
			sprintf(buf, "Starting in %d...", GAME->start_in);
			gfx_view_set_text(text_info, buf);
			break;
		case GAME_PLAYING:
			text_info->is_gone = false;
			sprintf(buf, "Match %d. Round: %d", GAME->match, GAME->round);
			gfx_view_set_text(text_info, buf);
			break;
		case GAME_FINISHED:
			players_title->is_gone = false;
			players_list->is_gone  = false;
			break;
	}
	ui->force_layout = true;
}

static void match_update_step_players(ui_t *ui) {}

// on_draw() will be called right after on_show()
static void on_draw(SDL_Renderer *renderer, view_t *view) {
	ui_t *ui = view->event.param;

	// first draw: recreate all used textures
	if (first_draw) {
		first_draw = false;
		SDL_DestroyTexture(canvas->data.canvas.texture);
		canvas->data.canvas.texture = SDL_CreateTexture(
			renderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET,
			SETTINGS->screen.width,
			SETTINGS->screen.height
		);
		SDL_SetTextureBlendMode(canvas->data.canvas.texture, SDL_BLENDMODE_BLEND);

		player_t *player;
		DL_FOREACH(GAME->players, player) {
			if (player->state == PLAYER_IDLE)
				continue;
			SDL_DestroyTexture(player->texture);
			player->texture = SDL_CreateTexture(
				renderer,
				SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,
				SETTINGS->screen.width,
				SETTINGS->screen.height
			);
			SDL_SetTextureBlendMode(player->texture, SDL_BLENDMODE_BLEND);
		}

		// make the UI redraw everything
		match_update_redraw_all(ui);
	}

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, canvas->data.canvas.texture, NULL, NULL);
	player_t *player;
	DL_FOREACH(GAME->players, player) {
		if (player->texture == NULL)
			continue;
		SDL_RenderCopy(renderer, player->texture, NULL, NULL);
	}
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
					return true;
				case MATCH_UPDATE_STEP_PLAYERS:
					match_update_step_players(ui);
					return true;
			}
			return false;
	}
	return false;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_match = {
	.on_show		  = on_show,
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
