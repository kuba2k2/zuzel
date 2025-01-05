// Copyright (c) Kuba Szczodrzyński 2025-1-5.

#include "game.h"

void game_request_update(game_t *game) {
	// request a global state update
	pkt_send_update_t pkt = {
		.hdr.type = PKT_SEND_UPDATE,
	};
	game_send_packet_pipe(game, (pkt_t *)&pkt);
}

void game_send_update(game_t *game, net_endpoint_t *source, net_endpoint_t *target) {
	int players;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, players);
	}

	pkt_game_data_t pkt = {
		.hdr.type  = PKT_GAME_DATA,
		.is_list   = target != NULL,
		.is_public = game->is_public,
		.speed	   = game->speed,
		.state	   = game->state,
		.players   = players - 1, // all except the pipe
	};
	strncpy(pkt.key, game->key, GAME_KEY_LEN);
	strncpy(pkt.name, game->name, GAME_NAME_LEN);

	if (target != NULL) {
		net_pkt_send(target, (pkt_t *)&pkt);
	} else {
		game_send_packet_broadcast(game, (pkt_t *)&pkt, source);
	}
}
