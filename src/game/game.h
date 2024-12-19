// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef struct game_t game_t;

game_t *game_init();
void game_free(game_t *game);
