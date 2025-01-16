// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "game_t.h"

typedef union pkt_t pkt_t;
typedef struct pkt_game_data_t pkt_game_data_t;

// game.c
game_t *game_init(pkt_game_data_t *pkt_data);
void game_stop(game_t *game);
void game_stop_all();
void game_free(game_t *game);
uint32_t game_expiry_cb(uint32_t interval, game_t *game);
game_t *game_get_by_key(char *key);
game_t *game_get_list(SDL_mutex **mutex);

// data.c
void game_set_default_player_options(game_t *game);
int game_get_player_count(game_t *game);
void game_fill_data_pkt(game_t *game, pkt_game_data_t *pkt);
void game_request_send_data(game_t *game, bool send_game, bool send_players);

// network.c
void game_add_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_del_endpoint(game_t *game, net_endpoint_t *endpoint);

// utils.c
void game_print_error(game_err_t error);
bool game_send_error(net_endpoint_t *endpoint, game_err_t error);

// packet.c
bool game_process_packet(game_t *game, pkt_t *pkt, net_endpoint_t *source);
