// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "include.h"

bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return false;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	// request a global state update
	pkt_send_update_t pkt = {
		.hdr.type = PKT_SEND_UPDATE,
	};
	game_send_packet(game, (pkt_t *)&pkt);
	return true;
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		DL_DELETE(game->endpoints, endpoint);
	}
	net_endpoint_close(endpoint);
	free(endpoint);
}

void game_send_packet(game_t *game, pkt_t *pkt) {
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
