// Copyright (c) Kuba Szczodrzyński 2025-1-6.

#include "game.h"

void game_set_default_player_options(game_t *game) {
	SDL_WITH_MUTEX(game->mutex) {
		// generate a game name
		if (SETTINGS->game_name != NULL)
			strncpy2(game->name, SETTINGS->game_name, GAME_NAME_LEN);
		else
			snprintf(game->name, sizeof(game->name), "%s's Game", SETTINGS->player_name);

		// set player's own speed
		game->speed = SETTINGS->game_speed;
	}
}

int game_get_player_count(game_t *game) {
	int players = 0;
	SDL_WITH_MUTEX(game->mutex) {
		player_t *item;
		DL_COUNT(game->players, item, players);
	}
	return players;
}

player_t *game_get_player_by_id(game_t *game, unsigned int id) {
	player_t *player = NULL;
	// find player by ID
	SDL_WITH_MUTEX(game->mutex) {
		DL_SEARCH_SCALAR(game->players, player, id, id);
	}
	return player;
}

void game_request_send_update(game_t *game, bool updated_game, unsigned int updated_player) {
	pkt_request_send_data_t pkt = {
		.hdr.type		= PKT_REQUEST_SEND_DATA,
		.updated_game	= updated_game,
		.updated_player = updated_player,
	};
	net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
}

void game_request_time_sync(game_t *game) {
	pkt_request_send_data_t pkt = {
		.hdr.type = PKT_REQUEST_TIME_SYNC,
	};
	net_pkt_send_pipe(game->endpoints, (pkt_t *)&pkt);
}

void game_fill_data_pkt(game_t *game, pkt_game_data_t *pkt) {
	pkt->is_public = game->is_public;
	pkt->is_local  = game->is_local;
	pkt->speed	   = game->speed;
	pkt->state	   = game->state;
	pkt->players   = game_get_player_count(game);
	memcpy(pkt->key, game->key, min(sizeof(pkt->key), sizeof(game->key)));
	memcpy(pkt->name, game->name, min(sizeof(pkt->name), sizeof(game->name)));
}
