// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "include.h"

bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return false;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	if (endpoint->type <= NET_ENDPOINT_TLS) {
		// request a global state update
		pkt_send_update_t pkt = {
			.hdr.type = PKT_SEND_UPDATE,
		};
		game_send_packet_pipe(game, (pkt_t *)&pkt);
	}
	return true;
}

void game_del_endpoint(game_t *game, net_endpoint_t *endpoint) {
	SDL_WITH_MUTEX(game->mutex) {
		DL_DELETE(game->endpoints, endpoint);
	}
	net_endpoint_close(endpoint);
	free(endpoint);
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
			LT_D("Game: broadcasting to %p", endpoint);
			net_pkt_send(endpoint, pkt);
		}
	}
}

void game_send_update(game_t *game, net_endpoint_t *source, net_endpoint_t *target) {
	pkt_game_data_t pkt = {
		.hdr.type  = PKT_GAME_DATA,
		.is_public = game->is_public,
		.speed	   = game->speed,
	};
	strncpy(pkt.key, game->key, GAME_KEY_LEN);
	strncpy(pkt.name, game->name, GAME_NAME_LEN);

	if (target != NULL) {
		net_pkt_send(target, (pkt_t *)&pkt);
	} else {
		game_send_packet_broadcast(game, (pkt_t *)&pkt, source);
	}
}
