// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "font_priv.h"

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

font_t *font_load_from_file(const char *filename) {
	FILE *file;
	FOPEN(file, filename, "rb", return NULL);

	// read font signature
	uint8_t buf[256];
	FREAD(file, buf, sizeof(buf), goto error);
	// check the header, read data depending on type
	font_t *font = NULL;
	if (memcmp(buf, "PK", 2) == 0) {
		font = font_bgi_load_from_file(file, buf, sizeof(buf));
	} else if (memcmp(buf, "BF", 2) == 0) {
		font = font_bmp_load_from_file(file, buf, sizeof(buf));
	} else {
		LT_E("Font signature invalid (%02x %02x)", buf[0], buf[1]);
		goto error;
	}
	if (font == NULL)
		goto error;

	// set default settings
	font->scale[0]	 = 1;
	font->scale[1]	 = 1;
	font->align_horz = FONT_ALIGN_LEFT;
	font->align_vert = FONT_ALIGN_TOP;

	fclose(file);
	return font;

error:
	fclose(file);
	return NULL;
}

void font_set_size(font_t *font, int size) {
	if (size > 0 && size < 10) {
		font->scale[0] = CHAR_SCALE[size][0];
		font->scale[1] = CHAR_SCALE[size][1];
	} else {
		LT_W("Font size %d not within bounds 1..9", size);
	}
}

void font_set_size_custom(font_t *font, int8_t mul, int8_t div) {
	font->scale[0] = mul;
	font->scale[1] = div;
}

void font_set_align(font_t *font, int horz, int vert) {
	font->align_horz = horz;
	font->align_vert = vert;
}

void font_draw_string(SDL_Renderer *renderer, int xc, int yc, const font_t *font, const char *s) {
	int xc_init = xc;
	font_align_string(font, &xc, &yc, s);
	char ch;
	while ((ch = *s++) != '\0') {
		if (ch == '\n') {
			yc += font->func.get_line_height(font) + SCALE(1); // + line spacing
			xc = xc_init;
			font_align_string(font, &xc, NULL, s);
		} else {
			xc += font->func.draw_char(renderer, xc, yc, font, ch);
		}
	}
}

int font_get_string_width(const font_t *font, const char *s) {
	char ch;
	int total_width = 0;
	while ((ch = *s++) != '\0') {
		if (ch == '\n')
			return total_width;
		total_width += font->func.get_char_width(font, ch);
	}
	return total_width;
}

int font_get_string_height(const font_t *font, const char *s) {
	char ch;
	int line_height	 = font->func.get_line_height(font);
	int total_height = line_height;
	while ((ch = *s++) != '\0') {
		if (ch == '\n')
			total_height += line_height + SCALE(1); // + line spacing
	}
	return total_height;
}

void font_align_string(const font_t *font, int *xc, int *yc, const char *s) {
	if (xc != NULL && font->align_horz != FONT_ALIGN_LEFT) {
		int width = font_get_string_width(font, s);
		if (font->align_horz == FONT_ALIGN_RIGHT) {
			*xc -= width;
		} else {
			*xc -= width / 2;
		}
	}
	if (yc != NULL && font->align_vert != FONT_ALIGN_TOP) {
		int height = font_get_string_height(font, s);
		if (font->align_vert == FONT_ALIGN_BOTTOM) {
			*yc -= height;
		} else {
			*yc -= height / 2;
		}
	}
}
