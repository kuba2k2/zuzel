// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "match_gfx.h"

static const SDL_Rect GAME_WALL_RECTS[] = {
	{.x = 150 - 1, .y = 90 - 1,	.w = 490 - 150 + 2, .h = 5},
	{.x = 150 - 1, .y = 190 - 1, .w = 490 - 150 + 2, .h = 5},
	{.x = 150 - 1, .y = 288 - 1, .w = 490 - 150 + 2, .h = 5},
	{.x = 150 - 1, .y = 388 - 1, .w = 490 - 150 + 2, .h = 5},
};
static const SDL_Rect GAME_TRACK_RECTS[] = {
	{.x = 150 - 1, .y = 90 - 1 + 5,	.w = 490 - 150 + 2, .h = 100 - 5},
	{.x = 150 - 1, .y = 288 - 1 + 5, .w = 490 - 150 + 2, .h = 100 - 5},
};
static const SDL_Point GAME_GATE_LINES[][2] = {
	{{.x = 305, .y = 320}, {.x = 320, .y = 320}},
	{{.x = 305, .y = 340}, {.x = 320, .y = 340}},
	{{.x = 305, .y = 360}, {.x = 320, .y = 360}},
	{{.x = 321, .y = 292}, {.x = 321, .y = 386}},
};

void match_board_draw(SDL_Renderer *renderer) {
	// draw board background
	gfx_set_color(renderer, GFX_COLOR_BLUE);
	SDL_RenderClear(renderer);

	// draw wall borders
	gfx_set_color(renderer, GFX_COLOR_BRIGHT_WHITE);
	SDL_RenderFillRects(renderer, GAME_WALL_RECTS, sizeof(GAME_WALL_RECTS) / sizeof(*GAME_WALL_RECTS));
	// - outer wall arc
	gfx_draw_half_circle(renderer, 150, 240, 90, 150, 5);
	gfx_draw_half_circle(renderer, 489, 240, 270, 150, 5);
	// - inner wall arc
	gfx_draw_half_circle(renderer, 150, 240, 90, 50, 5);
	gfx_draw_half_circle(renderer, 489, 240, 270, 50, 5);

	// draw track background
	gfx_set_color(renderer, GFX_COLOR_BLACK);
	for (int i = 150 - 5; i >= 50 + 5; i--) {
		gfx_draw_half_circle(renderer, 150, 240, 90, i, 5);
		gfx_draw_half_circle(renderer, 489, 240, 270, i, 5);
	}
	SDL_RenderFillRects(renderer, GAME_TRACK_RECTS, sizeof(GAME_TRACK_RECTS) / sizeof(*GAME_TRACK_RECTS));
}

void match_board_draw_gates(SDL_Renderer *renderer, bool show) {
	gfx_set_color(renderer, show ? GFX_COLOR_BRIGHT_MAGENTA : GFX_COLOR_BLACK);
	for (int i = 0; i < sizeof(GAME_GATE_LINES) / sizeof(GAME_GATE_LINES[0]); i++) {
		SDL_RenderDrawLines(renderer, GAME_GATE_LINES[i], sizeof(GAME_GATE_LINES[i]) / sizeof(*GAME_GATE_LINES[i]));
	}
}

void match_player_draw(SDL_Renderer *renderer, player_t *player) {
	if (player->state == PLAYER_IDLE)
		return;
	// render the player's line
	double prev_x = 0.0, prev_y = 0.0;
	gfx_set_color(renderer, player->color);
	for (player_pos_t *pos = &player->pos[0]; pos < player->pos + PLAYER_POS_NUM; pos++) {
		if (pos->x == prev_x && pos->y == prev_y)
			continue;
		if (pos != &player->pos[0]) {
			gfx_draw_line(renderer, (int)round(prev_x), (int)round(prev_y), (int)round(pos->x), (int)round(pos->y), 3);
		}
		prev_x = pos->x;
		prev_y = pos->y;
	}
}
