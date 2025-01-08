// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

static void game_check_empty(game_t *game);
static uint32_t game_expiry_cb(uint32_t interval, game_t *game);

void game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS && !game->is_client)
		// clients don't send endpoint connection updates
		game_request_send_game_data(game);
	// cancel the expiry timer
	game_check_empty(game);
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		DL_DELETE(game->endpoints, endpoint);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS && !game->is_client)
		// clients don't send endpoint connection updates
		game_request_send_game_data(game);
	net_endpoint_free(endpoint);
	free(endpoint);
	// stop the game if there are no more endpoints
	game_check_empty(game);
}

static void game_check_empty(game_t *game) {
	int endpoints = 0;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, endpoints);
		if (game->expiry_timer != 0) {
			LT_I("Game: clearing expiry timer");
			SDL_RemoveTimer(game->expiry_timer);
			game->expiry_timer = 0;
		}
		if (endpoints == 1) {
			LT_I("Game: empty, setting expiry timer");
			game->expiry_timer = SDL_AddTimer(60000, (SDL_TimerCallback)game_expiry_cb, game);
		}
	}
}

static uint32_t game_expiry_cb(uint32_t interval, game_t *game) {
	LT_I("Game: expired '%s' (key: %s)", game->name, game->key);
	SDL_WITH_MUTEX(game->mutex) {
		game->stop = true;
	}
	// wake up the game thread
	game_request_send_game_data(game);
	return 0;
}

void game_send_packet_pipe(game_t *game, pkt_t *pkt) {
	net_endpoint_t *pipe = NULL;
	SDL_WITH_MUTEX(game->mutex) {
		DL_FOREACH(game->endpoints, pipe) {
			if (pipe->type == NET_ENDPOINT_PIPE)
				break;
		}
	}
	if (pipe != NULL) {
		net_pkt_send(pipe, pkt);
	}
}
