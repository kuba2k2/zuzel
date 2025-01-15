// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#pragma once

#include "include.h"

#include "player_t.h"

typedef struct pkt_player_new_t pkt_player_new_t;
typedef struct pkt_player_data_t pkt_player_data_t;

// player.c
player_t *player_init(game_t *game, net_endpoint_t *endpoint, char *name);
void player_free(player_t *player);

// utils.c
void player_fill_data_pkt(game_t *game, player_t *player, pkt_player_data_t *pkt);
