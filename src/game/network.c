// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

void game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS)
		game_request_update(game);
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		DL_DELETE(game->endpoints, endpoint);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS)
		game_request_update(game);
	net_endpoint_free(endpoint);
	free(endpoint);
	// stop the game if there are no more endpoints
	int endpoints;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, endpoints);
	}
	if (endpoints == 1)
		game->stop = true;
}

void game_send_packet_pipe(game_t *game, pkt_t *pkt) {
	net_endpoint_t *pipe;
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

void game_send_packet_broadcast(game_t *game, pkt_t *pkt, net_endpoint_t *source) {
	net_endpoint_t *endpoint;
	SDL_WITH_MUTEX(game->mutex) {
		DL_FOREACH(game->endpoints, endpoint) {
			if (endpoint == source)
				continue;
			LT_D("Game: broadcasting to %s", net_endpoint_str(endpoint));
			net_pkt_send(endpoint, pkt);
		}
	}
}
