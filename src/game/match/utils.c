// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "match.h"

bool match_check_ready(game_t *game) {
	int players_count = 0;
	int ready_count	  = 0;
	player_t *player;
	DL_FOREACH(game->players, player) {
		if (player->state == PLAYER_SPECTATING || player->state == PLAYER_DISCONNECTED)
			// ignore spectating and disconnected players - they can't set READY
			continue;
		players_count++;
		if (player->state == PLAYER_READY)
			ready_count++;
	}
	return players_count != 0 && players_count == ready_count;
}

bool match_wait_ready(game_t *game) {
	if (match_check_ready(game))
		return true;
	LT_I("Match: players are not ready");
	do {
		SDL_SemWait(game->ready_sem);
	} while (!match_check_ready(game));
	return true;
}

void match_send_sdl_event(game_t *game, int code) {
	if (game->is_server)
		return;
	SDL_Event user = {
		.user.type = SDL_USEREVENT_MATCH,
		.user.code = code,
	};
	SDL_PushEvent(&user);
}
