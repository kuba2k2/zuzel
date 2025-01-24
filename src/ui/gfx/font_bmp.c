// Copyright (c) Kuba Szczodrzyński 2024-12-9.

#include "font_priv.h"

int font_bmp_draw_char(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch);
int font_bmp_get_char_width(const font_t *font, char ch);
int font_bmp_get_line_height(const font_t *font);

font_t *font_bmp_load(FILE *file, const uint8_t *data, const uint8_t *hdr, size_t hdr_len) {
	// allocate memory
	font_t *font = NULL;
	MALLOC(font, sizeof(*font), return NULL);
	font->type				   = FONT_TYPE_BMP;
	font->func.draw_char	   = font_bmp_draw_char;
	font->func.get_char_width  = font_bmp_get_char_width;
	font->func.get_line_height = font_bmp_get_line_height;

	if (file != NULL) {
		// read font header
		FSEEK(file, 2, SEEK_SET, goto error);
		FREAD(file, &font->bmp.font_hdr, sizeof(font->bmp.font_hdr), goto error);
		// read character data
		size_t line_size = (font->bmp.font_hdr.width - 1) / 8 + 1;
		size_t char_size = font->bmp.font_hdr.height * line_size;
		size_t data_size = font->bmp.font_hdr.char_count * char_size;
		MALLOC(font->bmp.data, data_size, goto error);
		FREAD(file, font->bmp.data, data_size, goto error);
	} else {
		// read font header
		memcpy(&font->bmp.font_hdr, &data[2], sizeof(font->bmp.font_hdr));
		// read character data
		size_t line_size = (font->bmp.font_hdr.width - 1) / 8 + 1;
		size_t char_size = font->bmp.font_hdr.height * line_size;
		size_t data_size = font->bmp.font_hdr.char_count * char_size;
		MALLOC(font->bmp.data, data_size, goto error);
		memcpy(font->bmp.data, &data[2 + sizeof(font->bmp.font_hdr)], data_size);
	}

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
#if MSVC
	SDL_Point *points = _malloca(SCALE(width * height) * 2 * sizeof(SDL_Point));
#else
	SDL_Point points[SCALE(width * height) * 2];
#endif
	SDL_Point *point = points;

	uint8_t *data = font->bmp.data[ch - font->bmp.font_hdr.char_start];
	int byte	  = *data++;

	int prev_sx = 0, prev_sy = 0;
	bool scale_2x = font->scale[0] == 2 && font->scale[1] == 1;
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
				if (scale_2x) {
					// optimize drawing 2x scaled fonts
					for (int i = 0; i < 4; i++) {
						point->x = xc + sx + (i & 1);
						point->y = yc + sy + (i >> 1);
						point++;
					}
				} else {
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
			}
			prev_sx = sx + 1;
		}
		prev_sy = sy + 1;
		// advance to the next line
		byte = *data++;
	}

	SDL_RenderDrawPoints(renderer, points, (int)(point - points));

	return SCALE(font->bmp.font_hdr.width);
}

int font_bmp_get_char_width(const font_t *font, char ch) {
	return SCALE(font->bmp.font_hdr.width);
}

int font_bmp_get_line_height(const font_t *font) {
	return SCALE(font->bmp.font_hdr.height);
}
