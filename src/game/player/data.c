// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#include "player.h"

void player_fill_data_pkt(game_t *game, player_t *player, pkt_player_data_t *pkt) {
	pkt->id	   = player->id;
	pkt->color = player->color;
	pkt->state = player->state;
	strcpy(pkt->name, player->name);
}
