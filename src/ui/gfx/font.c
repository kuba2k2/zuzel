// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "font_priv.h"

static font_t *FONTS[GFX_MAX_FONTS] = {0};
static font_t *FONT					= NULL;

/**
 * Load a font file to the memory.
 *
 * @param index destination index of the font
 * @param filename file to load from
 * @return font_t* if successful, NULL otherwise
 */
font_t *gfx_load_font(int index, const char *filename) {
	if (FONTS[index] != NULL) {
		LT_E("Font at index %d already loaded", index);
		return false;
	}
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
	font->scale[0] = 1;
	font->scale[1] = 1;
	font->align	   = GFX_ALIGN_DEFAULT;

	FONTS[index] = font;
	if (FONT == NULL)
		FONT = font;

	fclose(file);
	return font;

error:
	fclose(file);
	return NULL;
}

/**
 * Get a loaded font handle.
 *
 * @param index font index
 * @return font_t* if loaded, NULL otherwise
 */
font_t *gfx_get_font(int index) {
	return FONTS[index];
}

/**
 * Change the currently active font.
 *
 * @param index font index
 * @return font_t* if index valid, NULL otherwise
 */
font_t *gfx_set_text_font(int index) {
	// load font
	if (FONTS[index] == NULL) {
		LT_E("Font at index %d is not loaded", index);
		return NULL;
	}
	return FONT = FONTS[index];
}

/**
 * Change the currently active font and set its text style.
 *
 * @param index font index
 * @param size font size index (1..9), 4: default
 * @param align font alignment modes (GFX_ALIGN_*)
 */
void gfx_set_text_style(int index, int size, int align) {
	font_t *font = gfx_set_text_font(index);
	if (font == NULL)
		return;
	// set size
	if (size > 0 && size < 10) {
		font->scale[0] = CHAR_SCALE[size][0];
		font->scale[1] = CHAR_SCALE[size][1];
	} else {
		LT_W("Font size %d not within bounds 1..9", size);
	}
	// set alignment
	font->align = align;
}

/**
 * Calculate the width of the text. If multiline, the maximum width is returned, unless 'first_line' is true.
 *
 * @param s string to calculate
 * @param first_line whether to calculate the first line's width only
 * @return width of the text, in pixels
 */
int gfx_get_text_width(const char *s, bool first_line) {
	font_t *font = FONT;
	if (s == NULL || font == NULL)
		return 0;
	char ch;
	int max_width	= 0;
	int total_width = 0;
	while ((ch = *s++) != '\0') {
		if (ch == '\n') {
			if (first_line)
				return total_width;
			max_width	= max(max_width, total_width);
			total_width = 0;
		}
		total_width += font->func.get_char_width(font, ch);
	}
	return max(max_width, total_width);
}

/**
 * Calculate the height of the text. If multiline, the total height is calculated.
 *
 * @param s string to calculate
 * @return height of the text, in pixels
 */
int gfx_get_text_height(const char *s) {
	font_t *font = FONT;
	if (s == NULL || font == NULL)
		return 0;
	char ch;
	int line_height	 = font->func.get_line_height(font);
	int total_height = line_height;
	while ((ch = *s++) != '\0') {
		if (ch == '\n')
			total_height += line_height + SCALE(1); // + line spacing
	}
	return total_height;
}

static void gfx_text_align(const font_t *font, int *xc, int *yc, const char *s) {
	if (s == NULL || font == NULL)
		return;
	if (font->align & (GFX_ALIGN_RIGHT | GFX_ALIGN_CENTER_HORIZONTAL)) {
		int width = gfx_get_text_width(s, true);
		if (font->align & GFX_ALIGN_RIGHT) {
			*xc -= width;
		} else {
			*xc -= width / 2;
		}
	}
	if (yc != NULL && font->align & (GFX_ALIGN_BOTTOM | GFX_ALIGN_CENTER_VERTICAL)) {
		int height = gfx_get_text_height(s);
		if (font->align & GFX_ALIGN_BOTTOM) {
			*yc -= height;
		} else {
			*yc -= height / 2;
		}
	}
}

/**
 * Draw a string using the currently active font.
 *
 * @param renderer SDL renderer
 * @param xc X anchor
 * @param yc Y anchor
 * @param s string to draw
 * @return the new cursor X position
 */
int gfx_draw_text(SDL_Renderer *renderer, int xc, int yc, const char *s) {
	font_t *font = FONT;
	if (s == NULL || font == NULL)
		return xc;
	int xc_init = xc;
	gfx_text_align(font, &xc, &yc, s);
	char ch;
	while ((ch = *s++) != '\0') {
		if (ch == '\n') {
			yc += font->func.get_line_height(font) + SCALE(1); // + line spacing
			xc = xc_init;
			gfx_text_align(font, &xc, NULL, s);
		} else {
			xc += font->func.draw_char(renderer, xc, yc, font, ch);
		}
	}
	return xc;
}
