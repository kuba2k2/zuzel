// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#pragma once

#include "include.h"

typedef struct game_t game_t;

// match.c
bool match_init(game_t *game);

// utils.c
bool match_check_ready(game_t *game);
bool match_wait_ready(game_t *game);
void match_send_sdl_event(game_t *game, int code);
