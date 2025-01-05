// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef union pkt_t pkt_t;
typedef struct pkt_game_data_t pkt_game_data_t;
typedef struct game_t game_t;

// game.c
game_t *game_init(pkt_game_data_t *pkt_data);
void game_free(game_t *game);
game_t *game_get_by_key(const char *key);
game_t *game_get_list(SDL_mutex **mutex);

// network.c
void game_add_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_del_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_send_packet_pipe(game_t *game, pkt_t *pkt);
void game_send_packet_broadcast(game_t *game, pkt_t *pkt, net_endpoint_t *source);

// update.c
void game_request_update(game_t *game);
void game_send_update(game_t *game, net_endpoint_t *source, net_endpoint_t *target);

// utils.c
void game_set_data_default(game_t *game);
void game_print_error(game_err_t error);

// packet.c
bool game_process_packet(game_t *game, pkt_t *pkt, net_endpoint_t *source);
