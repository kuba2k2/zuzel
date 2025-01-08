// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef union pkt_t pkt_t;
typedef struct pkt_game_data_t pkt_game_data_t;
typedef struct game_t game_t;

// game.c
game_t *game_init(pkt_game_data_t *pkt_data);
void game_stop(game_t *game);
void game_free(game_t *game);
game_t *game_get_by_key(char *key);
game_t *game_get_list(SDL_mutex **mutex);

// data.c
void game_set_default_player_options(game_t *game);
void game_fill_game_data(game_t *game, pkt_game_data_t *pkt);
void game_request_send_game_data(game_t *game);

// network.c
void game_add_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_del_endpoint(game_t *game, net_endpoint_t *endpoint);

// utils.c
void game_print_error(game_err_t error);

// packet.c
bool game_process_packet(game_t *game, pkt_t *pkt, net_endpoint_t *source);
