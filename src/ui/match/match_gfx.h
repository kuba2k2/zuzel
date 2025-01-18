// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#pragma once

#include "include.h"

void match_gfx_board_draw(SDL_Renderer *renderer);
void match_gfx_gates_draw(SDL_Renderer *renderer, bool show);
void match_gfx_player_draw(SDL_Renderer *renderer, player_t *player);
void match_gfx_player_draw_step(SDL_Renderer *renderer, player_t *player);
