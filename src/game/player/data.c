// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#include "player.h"

void player_fill_data_pkt(game_t *game, player_t *player, pkt_player_data_t *pkt) {
	pkt->id	   = player->id;
	pkt->color = player->color;
	pkt->state = player->state;
	strcpy(pkt->name, player->name);
}

void player_reset_round(game_t *game) {
	player_t *player, *player_0 = NULL, *player_1 = NULL;
	int player_count = 0;
	DL_FOREACH(game->players, player) {
		if (player->state != PLAYER_READY)
			continue;
		// find the 1st and 2nd players that are ready
		if (player_0 == NULL)
			player_0 = player;
		else if (player_1 == NULL)
			player_1 = player;
		player_count++;
	}
	if (player_count == 0)
		return;

	// generate Y positions
	player_0		   = game->players;
	player_0->pos[0].y = 310.0 + (rand() % 4) * 20.0;
	if (player_count > 2) {
		int player_idx = 1;
		DL_FOREACH(game->players, player) {
			if (player->state != PLAYER_READY)
				continue;
			player->pos[0].y = player_0->pos[0].y + (double)player_idx * 20.0;
			if (player->pos[0].y > 370.0)
				player->pos[0].y -= 80.0;
			player_idx++;
		}
	} else if (player_count == 2 && player_1 != NULL) {
		if (player_0->pos[0].y >= 350.0)
			player_1->pos[0].y = player_0->pos[0].y - 40.0;
		else
			player_1->pos[0].y = player_0->pos[0].y + 40.0;
	}

	DL_FOREACH(game->players, player) {
		if (player->state != PLAYER_READY)
			continue;
		// reset player data
		player->state			= PLAYER_PLAYING;
		player->lap				= 1;
		player->lap_can_advance = false;
		player->round_points	= 0;
		// reset all player positions
		player->pos[0].time		 = 0;
		player->pos[0].angle	 = 0;
		player->pos[0].speed	 = 1.0;
		player->pos[0].x		 = 320.0;
		player->pos[0].direction = PLAYER_POS_FORWARD;
		player->pos[0].confirmed = true;
		for (int i = 1; i < 20; i++) {
			player->pos[i]	 = player->pos[0];
			player->pos[i].x = 320 - (i + 1);
		}
		for (int i = 20; i < 100; i++) {
			player->pos[i]	 = player->pos[0];
			player->pos[i].x = 300;
		}
	}
}
