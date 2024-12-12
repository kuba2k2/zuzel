// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "include.h"

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

struct font_s;
typedef struct font_s font_t;

font_t *gfx_load_font(int index, const char *filename);
font_t *gfx_get_font(int index);
font_t *gfx_set_text_font(int index);
void gfx_set_text_style(int index, int size, int align);
int gfx_get_text_width(const font_t *font, const char *s);
int gfx_get_text_height(const font_t *font, const char *s);
void gfx_draw_text(SDL_Renderer *renderer, int xc, int yc, const char *s);
