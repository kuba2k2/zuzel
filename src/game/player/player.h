// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#pragma once

#include "include.h"

#include "player_t.h"

typedef struct pkt_player_new_t pkt_player_new_t;
typedef struct pkt_player_data_t pkt_player_data_t;

// player.c
player_t *player_init(game_t *game, char *name);
void player_free(player_t *player);
bool player_position_shift(player_t *player);
void player_position_calculate(player_t *player, player_pos_t *start);
bool player_position_check_lap(player_t *player, player_pos_t *prev, player_pos_t *next);
bool player_position_check_collision(player_t *player, player_pos_t *pos);
void player_position_remote_keypress(player_t *player, unsigned int time, player_pos_dir_t direction);
bool player_loop(player_t *player);

// data.c
void player_set_color(game_t *game, player_t *player);
void player_set_key(game_t *game, player_t *player);
void player_fill_data_pkt(game_t *game, player_t *player, pkt_player_data_t *pkt);
void player_reset_round(game_t *game);
