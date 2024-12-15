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

#define FONT_SIZE_DEFAULT 4

#define FONT_ALIGN_LEFT_TOP		 0x00
#define FONT_ALIGN_LEFT_MIDDLE	 0x01
#define FONT_ALIGN_LEFT_BOTTOM	 0x02
#define FONT_ALIGN_CENTER_TOP	 0x10
#define FONT_ALIGN_CENTER_MIDDLE 0x11
#define FONT_ALIGN_CENTER_BOTTOM 0x12
#define FONT_ALIGN_RIGHT_TOP	 0x20
#define FONT_ALIGN_RIGHT_MIDDLE	 0x21
#define FONT_ALIGN_RIGHT_BOTTOM	 0x22

typedef struct font_t font_t;

// gfx.c
void gfx_set_color(SDL_Renderer *renderer, int color);
void gfx_set_color_alpha(SDL_Renderer *renderer, int color, int alpha);
void gfx_draw_rect_points(SDL_Renderer *renderer, SDL_Rect *rects, int count, int width);
void gfx_draw_half_circle(SDL_Renderer *renderer, int cx, int cy, int angle, int radius, int width);
void gfx_draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int width);

// font.c
font_t *gfx_load_font(int index, const char *filename);
font_t *gfx_get_font(int index);
font_t *gfx_set_text_font(int index);
void gfx_set_text_style(int index, int size, int align);
int gfx_get_text_width(const font_t *font, const char *s);
int gfx_get_text_height(const font_t *font, const char *s);
void gfx_draw_text(SDL_Renderer *renderer, int xc, int yc, const char *s);
