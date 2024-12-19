// Copyright (c) Kuba Szczodrzyński 2024-12-14.

#pragma once

#include "include.h"

#include "structs.h"

typedef struct menu_t menu_t;

menu_t *menu_init(SDL_Renderer *renderer);
int menu_run(menu_t *menu, game_t **game);
void menu_free(menu_t *menu);
