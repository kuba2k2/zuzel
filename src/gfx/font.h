// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "include.h"

#define FONT_SIZE_DEFAULT 4
#define FONT_ALIGN_CENTER 0
#define FONT_ALIGN_LEFT	  (-1) // default
#define FONT_ALIGN_RIGHT  1
#define FONT_ALIGN_BOTTOM (-1)
#define FONT_ALIGN_TOP	  1 // default

struct font_s;
typedef struct font_s font_t;

font_t *font_load_from_file(const char *filename);
void font_set_size(font_t *font, int size);
void font_set_size_custom(font_t *font, int8_t mul, int8_t div);
void font_set_align(font_t *font, int horz, int vert);
void font_draw_string(SDL_Renderer *renderer, int xc, int yc, const font_t *font, const char *s);
int font_get_string_width(const font_t *font, const char *s);
