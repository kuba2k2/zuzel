// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

typedef struct race_t race_t;

race_t *race_init(SDL_Renderer *renderer, game_t *game);
int race_run(race_t *race);
void race_free(race_t *race);
