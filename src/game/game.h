// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef union pkt_t pkt_t;

typedef enum {
	GAME_ERR_OK			   = 0, //!< No error
	GAME_ERR_INVALID_STATE = 1, //!< Operation invalid in the current game state
	GAME_ERR_NOT_FOUND	   = 2, //!< Game not found by the specified key
} game_err_t;

typedef struct game_t game_t;

// game.c
game_t *game_init();
void game_free(game_t *game);
game_t *game_get_by_key(const char *key);
game_t *game_get_list(SDL_mutex **mutex);

// utils.c
bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_del_endpoint(game_t *game, net_endpoint_t *endpoint);
void game_send_packet_pipe(game_t *game, pkt_t *pkt);
void game_send_packet_broadcast(game_t *game, pkt_t *pkt, net_endpoint_t *source);
void game_send_update(game_t *game, net_endpoint_t *source, net_endpoint_t *target);
void game_print_error(game_err_t error);
