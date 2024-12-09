// Copyright (c) Kuba Szczodrzyński 2024-12-9.

#include "font_priv.h"

int font_bmp_draw_char(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch);
int font_bmp_get_char_width(const font_t *font, char ch);
void font_bmp_align_string(const font_t *font, int *xc, int *yc, const char *s);

font_t *font_bmp_load_from_file(FILE *file, const uint8_t *hdr, size_t hdr_len) {
	// allocate memory
	font_t *font = NULL;
	MALLOC(font, sizeof(*font), return NULL);
	font->type				  = FONT_TYPE_BMP;
	font->func.draw_char	  = font_bmp_draw_char;
	font->func.get_char_width = font_bmp_get_char_width;
	font->func.align_string	  = font_bmp_align_string;

	// read font header
	FSEEK(file, 2, SEEK_SET, goto error);
	FREAD(file, &font->bmp.font_hdr, sizeof(font->bmp.font_hdr), goto error);
	// read character data
	size_t line_size = (font->bmp.font_hdr.width - 1) / 8 + 1;
	size_t char_size = font->bmp.font_hdr.height * line_size;
	size_t data_size = font->bmp.font_hdr.char_count * char_size;
	MALLOC(font->bmp.data, data_size, goto error);
	FREAD(file, font->bmp.data, data_size, goto error);

	LT_I(
		"Loaded bitmap font '%c%c%c%c' with %u characters",
		font->bmp.font_hdr.font_name[0],
		font->bmp.font_hdr.font_name[1],
		font->bmp.font_hdr.font_name[2],
		font->bmp.font_hdr.font_name[3],
		font->bmp.font_hdr.char_count
	);

	return font;

error:
	free(font->bmp.data);
	free(font);
	return NULL;
}

int font_bmp_draw_char(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch) {
	int width  = font->bmp.font_hdr.width;
	int height = font->bmp.font_hdr.height;
	SDL_Point points[SCALE(width * height)];
	SDL_Point *point = points;

	uint8_t *data = font->bmp.data[ch - font->bmp.font_hdr.char_start];
	int byte	  = *data++;

	int prev_sx = 0, prev_sy = 0;
	for (int y = 0; y < height; y++) {
		int sy = SCALE(y);
		for (int x = 0; x < width; x++) {
			int sx = SCALE(x);
			// calculate bit index
			int bit = x % 8;
			if (x && bit == 0)
				// read next data byte (if width > 8)
				byte = *data++;
			// check if pixel is enabled
			if (byte & (0x80 >> bit)) {
				// draw a point for the pixel
				point->x = xc + sx;
				point->y = yc + sy;
				point++;
				if (x != sx || y != sy) {
					// perform "scaling" by tracing a path from the previous adjacent pixel
					for (int px = prev_sx; px < sx; px++) {
						point->x = xc + px;
						point->y = yc + sy;
						point++;
						for (int py = prev_sy; py < sy; py++) {
							point->x = xc + px;
							point->y = yc + py;
							point++;
						}
					}
					for (int py = prev_sy; py < sy; py++) {
						point->x = xc + sx;
						point->y = yc + py;
						point++;
					}
				}
			}
			prev_sx = sx + 1;
		}
		prev_sy = sy + 1;
		// advance to the next line
		byte	= *data++;
	}

	SDL_RenderDrawPoints(renderer, points, (int)(point - points));

	return SCALE(font->bmp.font_hdr.width);
}

int font_bmp_get_char_width(const font_t *font, char ch) {
	return SCALE(font->bmp.font_hdr.width);
}

void font_bmp_align_string(const font_t *font, int *xc, int *yc, const char *s) {
	if (font->align_horz != FONT_ALIGN_LEFT) {
		int width = font_get_string_width(font, s);
		if (font->align_horz == FONT_ALIGN_RIGHT) {
			*xc -= width;
		} else {
			*xc -= width / 2;
		}
	}
	if (font->align_vert == FONT_ALIGN_BOTTOM) {
		*yc -= SCALE(font->bmp.font_hdr.height);
	} else if (font->align_vert == FONT_ALIGN_CENTER) {
		*yc -= SCALE(font->bmp.font_hdr.height) / 2;
	}
}
