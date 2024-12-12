// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "include.h"

#define GFX_COLOR_BLACK			 0
#define GFX_COLOR_BLUE			 1
#define GFX_COLOR_GREEN			 2
#define GFX_COLOR_CYAN			 3
#define GFX_COLOR_RED			 4
#define GFX_COLOR_MAGENTA		 5
#define GFX_COLOR_YELLOW		 6
#define GFX_COLOR_WHITE			 7
#define GFX_COLOR_BRIGHT_BLACK	 8
#define GFX_COLOR_BRIGHT_BLUE	 9
#define GFX_COLOR_BRIGHT_GREEN	 10
#define GFX_COLOR_BRIGHT_CYAN	 11
#define GFX_COLOR_BRIGHT_RED	 12
#define GFX_COLOR_BRIGHT_MAGENTA 13
#define GFX_COLOR_BRIGHT_YELLOW	 14
#define GFX_COLOR_BRIGHT_WHITE	 15

void gfx_set_color(SDL_Renderer *renderer, int color);
void gfx_set_color_alpha(SDL_Renderer *renderer, int color, int alpha);
void gfx_draw_rect_points(SDL_Renderer *renderer, SDL_Rect *rects, int count, int width);
void gfx_draw_half_circle(SDL_Renderer *renderer, int cx, int cy, int angle, int radius, int width);
void gfx_draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int width);
