// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "fragment.h"

#include "ui/match/match_gfx.h"

static view_t *text_top			  = NULL;
static view_t *text_info		  = NULL;
static view_t *text_players_title = NULL;
static view_t *text_players_list  = NULL;

static game_t *game = NULL;

static void on_draw(SDL_Renderer *renderer, view_t *canvas);
static void on_error(ui_t *ui);

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	game = ui->game;
	if (game == NULL) {
		LT_E("Game is not provided");
		goto error;
	}

	view_t *canvas;
	GFX_VIEW_BIND(fragment->views, canvas, goto error);
	GFX_VIEW_BIND(fragment->views, text_top, goto error);
	GFX_VIEW_BIND(fragment->views, text_info, goto error);
	GFX_VIEW_BIND(fragment->views, text_players_title, goto error);
	GFX_VIEW_BIND(fragment->views, text_players_list, goto error);

	// destroy the old canvas texture
	SDL_DestroyTexture(canvas->data.canvas.texture);
	canvas->data.canvas.texture = NULL;
	canvas->draw				= on_draw;

	return true;

error:
	on_error(ui);
	return false;
}

static void on_draw(SDL_Renderer *renderer, view_t *canvas) {
	SDL_Texture *texture = canvas->data.canvas.texture;
	if (texture == NULL) {
		// create texture if not created yet
		texture = SDL_CreateTexture(
			renderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET,
			SETTINGS->screen.width,
			SETTINGS->screen.height
		);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_SetRenderTarget(renderer, texture);
		canvas->data.canvas.texture = texture;
		// render the board
		match_board_draw(renderer);
		match_board_draw_gates(renderer, true);
	}

	player_t *player;

	DL_FOREACH(game->players, player) {
		match_player_draw(renderer, player);
	}

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	DL_FOREACH(game->players, player) {
		SDL_RenderCopy(renderer, player->texture, NULL, NULL);
	}
}

static void on_error(ui_t *ui) {
	game_stop(game);
	net_client_stop();
	net_server_stop();
	ui_state_set(ui, UI_STATE_MAIN);
	ui_state_set(ui, UI_STATE_ERROR);
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_match = {
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
