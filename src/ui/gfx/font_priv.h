// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#pragma once

#include "gfx.h"

static const int8_t CHAR_SCALE[10][2] = {
	{00, 00}, // apply custom scale coefficients
	{03, 05},
	{02, 03},
	{03, 04},
	{01, 01}, // default size
	{04, 03},
	{05, 03},
	{02, 01},
	{05, 02},
	{03, 01}
};

#define SCALE(var) ((var) * font->scale[0] / font->scale[1])

#define FONT_ALIGN_TOP	  0
#define FONT_ALIGN_MIDDLE 1
#define FONT_ALIGN_BOTTOM 2
#define FONT_ALIGN_LEFT	  0
#define FONT_ALIGN_CENTER 1
#define FONT_ALIGN_RIGHT  2

font_t *font_bgi_load_from_file(FILE *file, const uint8_t *hdr, size_t hdr_len);
font_t *font_bmp_load_from_file(FILE *file, const uint8_t *hdr, size_t hdr_len);

typedef int (*font_draw_char_t)(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch);
typedef int (*font_get_char_width_t)(const font_t *font, char ch);
typedef int (*font_get_line_height_t)(const font_t *font);

typedef struct __attribute__((packed)) {
	uint8_t header_start;
	uint16_t header_size;
	char font_name[4];
	uint16_t font_size;
	uint8_t font_major;
	uint8_t font_minor;
	uint8_t rev_major;
	uint8_t rev_minor;
} bgi_font_header_t;

typedef struct __attribute__((packed)) {
	uint8_t stroke_check;
	uint16_t char_count;
	uint8_t _padding1;
	char char_start;
	uint16_t stroke_offset;
	uint8_t _padding2;
	int8_t origin_ascender;
	int8_t origin_baseline;
	int8_t origin_descender;
	uint32_t _padding3;
	uint8_t _padding4;
} bgi_stroke_header_t;

typedef struct __attribute__((packed)) {
	uint8_t x;
	uint8_t y;
} bgi_stroke_data_t;

typedef struct __attribute__((packed)) {
	uint16_t char_count;
	char font_name[4];
	uint8_t width;
	uint8_t height;
	char char_start;
	uint8_t _padding1;
} bmp_font_header_t;

typedef uint8_t bmp_char_data_t[8];

typedef enum {
	FONT_TYPE_BGI = 1,
	FONT_TYPE_BMP = 2,
} font_type_t;

typedef struct font_t {
	font_type_t type;

	union {
		struct {
			bgi_font_header_t font_hdr;
			bgi_stroke_header_t stroke_hdr;
			uint16_t *offsets;
			uint8_t *widths;
			bgi_stroke_data_t *strokes;
		} bgi;

		struct {
			bmp_font_header_t font_hdr;
			bmp_char_data_t *data;
		} bmp;
	};

	struct {
		font_draw_char_t draw_char;
		font_get_char_width_t get_char_width;
		font_get_line_height_t get_line_height;
	} func;

	// runtime settings
	int8_t scale[2];
	int align_horz;
	int align_vert;
} font_t;
