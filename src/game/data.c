// Copyright (c) Kuba Szczodrzyński 2025-1-6.

#include "game.h"

void game_set_default_player_options(game_t *game) {
	SDL_WITH_MUTEX(game->mutex) {
		// generate a game name
		if (SETTINGS->game_name != NULL)
			strncpy(game->name, SETTINGS->game_name, sizeof(game->name) - 1);
		else
			snprintf(game->name, sizeof(game->name), "%s's Game", SETTINGS->player_name);

		// set player's own speed
		game->speed = SETTINGS->game_speed;
	}
}

void game_request_send_game_data(game_t *game) {
	pkt_send_game_data_t pkt = {
		.hdr.type = PKT_SEND_GAME_DATA,
	};
	game_send_packet_pipe(game, (pkt_t *)&pkt);
}

void game_data_fill_pkt(game_t *game, pkt_game_data_t *pkt) {
	int endpoints;
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item;
		DL_COUNT(game->endpoints, item, endpoints);
	}

	pkt->is_public = game->is_public;
	pkt->speed	   = game->speed;
	pkt->state	   = game->state;
	pkt->players   = endpoints - 1; // all except the pipe
	strncpy(pkt->key, game->key, GAME_KEY_LEN);
	strncpy(pkt->name, game->name, GAME_NAME_LEN);
}
