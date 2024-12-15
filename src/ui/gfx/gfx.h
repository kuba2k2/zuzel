// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "include.h"

#define GFX_COLOR_BLACK			 0x000000
#define GFX_COLOR_BLUE			 0x0000AA
#define GFX_COLOR_GREEN			 0x00AA00
#define GFX_COLOR_CYAN			 0x00AAAA
#define GFX_COLOR_RED			 0xAA0000
#define GFX_COLOR_MAGENTA		 0xAA00AA
#define GFX_COLOR_YELLOW		 0xAAAA00
#define GFX_COLOR_WHITE			 0xAAAAAA
#define GFX_COLOR_BRIGHT_BLACK	 0x555555
#define GFX_COLOR_BRIGHT_BLUE	 0x5555FF
#define GFX_COLOR_BRIGHT_GREEN	 0x55FF55
#define GFX_COLOR_BRIGHT_CYAN	 0x55FFFF
#define GFX_COLOR_BRIGHT_RED	 0xFF5555
#define GFX_COLOR_BRIGHT_MAGENTA 0xFF55FF
#define GFX_COLOR_BRIGHT_YELLOW	 0xFFFF55
#define GFX_COLOR_BRIGHT_WHITE	 0xFFFFFF

#define GFX_ALIGN_LEFT				0x01
#define GFX_ALIGN_RIGHT				0x02
#define GFX_ALIGN_TOP				0x04
#define GFX_ALIGN_BOTTOM			0x08
#define GFX_ALIGN_CENTER_HORIZONTAL 0x10
#define GFX_ALIGN_CENTER_VERTICAL	0x20
#define GFX_ALIGN_DEFAULT			(GFX_ALIGN_LEFT | GFX_ALIGN_TOP)
#define GFX_ALIGN_CENTER			(GFX_ALIGN_CENTER_HORIZONTAL | GFX_ALIGN_CENTER_VERTICAL)

#define FONT_SIZE_DEFAULT 4

typedef struct font_t font_t;

// gfx.c
void gfx_set_color(SDL_Renderer *renderer, unsigned int color);
void gfx_draw_rect_points(SDL_Renderer *renderer, SDL_Rect *rects, int count, int width);
void gfx_draw_half_circle(SDL_Renderer *renderer, int cx, int cy, int angle, int radius, int width);
void gfx_draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int width);

// font.c
font_t *gfx_load_font(int index, const char *filename);
font_t *gfx_get_font(int index);
font_t *gfx_set_text_font(int index);
void gfx_set_text_style(int index, int size, int align);
int gfx_get_text_width(const char *s, bool first_line);
int gfx_get_text_height(const char *s);
void gfx_draw_text(SDL_Renderer *renderer, int xc, int yc, const char *s);
