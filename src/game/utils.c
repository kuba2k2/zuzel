// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "include.h"

bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return false;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	// wake up the game thread if possible
	net_endpoint_t *pipe;
	DL_FOREACH(game->endpoints, pipe) {
		if (pipe->type == NET_ENDPOINT_PIPE) {
			// request a global state update
			pkt_send_update_t pkt = {
				.hdr.type = PKT_SEND_UPDATE,
			};
			net_pkt_send(pipe, (pkt_t *)&pkt);
			break;
		}
	}
	return true;
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item, *tmp;
		DL_FOREACH_SAFE(game->endpoints, item, tmp) {
			if (item != endpoint)
				continue;
			DL_DELETE(game->endpoints, item);
		}
	}
	net_endpoint_close(endpoint);
	free(endpoint);
}
