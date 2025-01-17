// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#include "match.h"

bool match_check_ready(game_t *game) {
	int players_count = 0;
	int ready_count	  = 0;
	player_t *player;
	DL_FOREACH(game->players, player) {
		players_count++;
		if (player->state == PLAYER_READY)
			ready_count++;
	}
	return players_count != 0 && players_count == ready_count;
}

bool match_init(game_t *game) {
	if (game->is_server) {
		// signal clients that the game starts now
		pkt_game_start_t pkt = {
			.hdr.type = PKT_GAME_START,
		};
		net_pkt_broadcast(game->endpoints, (pkt_t *)&pkt, NULL);
	}

	LT_I("Match: starting match in game '%s' (%s)", game->name, game->key);
	game->state = GAME_STARTING;

	player_t *player;
	DL_FOREACH(game->players, player) {
		player->state = PLAYER_PLAYING;
	}

	return true;
}
