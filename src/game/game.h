// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef enum {
	GAME_ERR_OK			   = 0, //!< No error
	GAME_ERR_INVALID_STATE = 1, //!< Operation invalid in the current game state
} game_err_t;

typedef struct game_t game_t;

// game.c
game_t *game_init();
void game_free(game_t *game);

// utils.c
bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint);
