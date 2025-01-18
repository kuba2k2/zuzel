// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#pragma once

#include "include.h"

void match_board_draw(SDL_Renderer *renderer);
void match_board_draw_gates(SDL_Renderer *renderer, bool show);
void match_player_draw(SDL_Renderer *renderer, player_t *player);
