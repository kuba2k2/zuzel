// Copyright (c) Kuba Szczodrzyński 2025-1-6.

#include "game.h"

void game_set_data_default(game_t *game) {
	// generate a game name
	if (SETTINGS->game_name != NULL)
		strncpy(game->name, SETTINGS->game_name, sizeof(game->name) - 1);
	else
		snprintf(game->name, sizeof(game->name), "%s's Game", SETTINGS->player_name);

	// generate a game key
	do {
		char *ch = game->key;
		for (int i = 0; i < sizeof(game->key) - 1; i++) {
			int num = '0' + rand() % 36;
			if (num > '9')
				num += 'A' - '9' - 1;
			*ch++ = (char)num;
		}
	} while (game_get_by_key(game->key) != NULL);

	// set some other default settings
	game->is_public = false;
	game->speed		= 3;
	game->state		= GAME_IDLE;
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
