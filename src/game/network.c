// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

static void game_check_empty(game_t *game, bool endpoint_deleted);

void game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS && game->is_server)
		// clients don't send endpoint connection updates
		game_request_send_game_data(game);
	// cancel the expiry timer
	game_check_empty(game, false);
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		DL_DELETE(game->endpoints, endpoint);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS && game->is_server)
		// clients don't send endpoint connection updates
		game_request_send_game_data(game);
	net_endpoint_free(endpoint);
	free(endpoint);
	// stop the game if there are no more endpoints
	game_check_empty(game, true);
}

static void game_check_empty(game_t *game, bool endpoint_deleted) {
	int endpoints = 0;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, endpoints);
	}
	if (!endpoint_deleted && endpoints == 1)
		// avoid messing with the timer on pipe endpoint adding
		return;

	if (game->expiry_timer != 0) {
		LT_I("Game: clearing expiry timer");
		SDL_RemoveTimer(game->expiry_timer);
		game->expiry_timer = 0;
	}
	if (!endpoint_deleted)
		return;
	if (endpoints > 1)
		return;

	if (game->is_server && !game->is_local) {
		LT_I("Game: empty, setting expiry timer");
		game->expiry_timer = SDL_AddTimer(60000, (SDL_TimerCallback)game_expiry_cb, game);
	} else {
		// clients should immediately stop if the server disconnects
		LT_I("Game: empty, stopping immediately");
		game->stop = true;
	}
}
