// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "font_priv.h"

int font_bgi_draw_char(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch);
int font_bgi_get_char_width(const font_t *font, char ch);
int font_bgi_get_line_height(const font_t *font);

font_t *font_bgi_load(FILE *file, const uint8_t *data, const uint8_t *hdr, size_t hdr_len) {
	size_t header_pos = 0;
	for (; header_pos < hdr_len; header_pos++) {
		if (hdr[header_pos] == 0x1A)
			break;
	}
	if (header_pos == hdr_len) {
		LT_E("Font header not found");
		return NULL;
	}

	// allocate memory
	font_t *font = NULL;
	MALLOC(font, sizeof(*font), return NULL);
	font->type				   = FONT_TYPE_BGI;
	font->func.draw_char	   = font_bgi_draw_char;
	font->func.get_char_width  = font_bgi_get_char_width;
	font->func.get_line_height = font_bgi_get_line_height;

	if (file != NULL) {
		// read font header
		FSEEK(file, header_pos, SEEK_SET, goto error);
		FREAD(file, &font->bgi.font_hdr, sizeof(font->bgi.font_hdr), goto error);
		// read stroke header
		FSEEK(file, font->bgi.font_hdr.header_size, SEEK_SET, goto error);
		FREAD(file, &font->bgi.stroke_hdr, sizeof(font->bgi.stroke_hdr), goto error);
		// read character stroke offsets
		size_t offsets_size = font->bgi.stroke_hdr.char_count * sizeof(*font->bgi.offsets);
		MALLOC(font->bgi.offsets, offsets_size, goto error);
		FREAD(file, font->bgi.offsets, offsets_size, goto error);
		// read character widths
		size_t widths_size = font->bgi.stroke_hdr.char_count * sizeof(*font->bgi.widths);
		MALLOC(font->bgi.widths, widths_size, goto error);
		FREAD(file, font->bgi.widths, widths_size, goto error);
		// read stroke data
		size_t data_size = font->bgi.font_hdr.font_size;
		data_size -= sizeof(font->bgi.stroke_hdr);
		data_size -= offsets_size;
		data_size -= widths_size;
		MALLOC(font->bgi.strokes, data_size, goto error);
		FREAD(file, font->bgi.strokes, data_size, goto error);
	} else {
		// read font header
		const uint8_t *font_hdr = &data[header_pos];
		memcpy(&font->bgi.font_hdr, font_hdr, sizeof(font->bgi.font_hdr));
		// read stroke header
		const uint8_t *stroke_hdr = &data[font->bgi.font_hdr.header_size];
		memcpy(&font->bgi.stroke_hdr, stroke_hdr, sizeof(font->bgi.stroke_hdr));
		// read character stroke offsets
		size_t offsets_size	   = font->bgi.stroke_hdr.char_count * sizeof(*font->bgi.offsets);
		const uint8_t *offsets = stroke_hdr + sizeof(font->bgi.stroke_hdr);
		MALLOC(font->bgi.offsets, offsets_size, goto error);
		memcpy(font->bgi.offsets, offsets, offsets_size);
		// read character widths
		size_t widths_size	  = font->bgi.stroke_hdr.char_count * sizeof(*font->bgi.widths);
		const uint8_t *widths = offsets + offsets_size;
		MALLOC(font->bgi.widths, widths_size, goto error);
		memcpy(font->bgi.widths, widths, widths_size);
		// read stroke data
		size_t data_size = font->bgi.font_hdr.font_size;
		data_size -= sizeof(font->bgi.stroke_hdr);
		data_size -= offsets_size;
		data_size -= widths_size;
		const uint8_t *strokes = widths + widths_size;
		MALLOC(font->bgi.strokes, data_size, goto error);
		memcpy(font->bgi.strokes, strokes, data_size);
	}

	LT_I(
		"Loaded BGI font '%c%c%c%c' with %u characters",
		font->bgi.font_hdr.font_name[0],
		font->bgi.font_hdr.font_name[1],
		font->bgi.font_hdr.font_name[2],
		font->bgi.font_hdr.font_name[3],
		font->bgi.stroke_hdr.char_count
	);

	return font;

error:
	free(font->bgi.offsets);
	free(font->bgi.widths);
	free(font->bgi.strokes);
	free(font);
	return NULL;
}

int font_bgi_draw_char(SDL_Renderer *renderer, int xc, int yc, const font_t *font, char ch) {
	uint16_t stroke_offs		  = font->bgi.offsets[ch - font->bgi.stroke_hdr.char_start];
	const bgi_stroke_data_t *data = &font->bgi.strokes[stroke_offs / sizeof(bgi_stroke_data_t)];

	// compensate for stroke offset
	xc += 1;
	yc += SCALE(font->bgi.stroke_hdr.origin_ascender);

	int x = xc, y = yc;
	while (true) {
		/* 0 - stop; 1 - scan; 2 - move; 3 - draw */
		int opcode = ((data->x >> 7) << 1) + (data->y >> 7);
		if (opcode == 0)
			break;
		if (opcode == 1) {
			data++;
			continue;
		}
		// bit arithmetic (note dx/dy signed 8 bit type):
		// - if sign bit is set - expand to full signed value
		int8_t dx = (int8_t)((data->x & 0x7F) | ((data->x & 0x40) << 1));
		int8_t dy = (int8_t)-((data->y & 0x7F) | ((data->y & 0x40) << 1));
		// apply scale coefficient
		dx = (int8_t)SCALE(dx);
		dy = (int8_t)SCALE(dy);
		// dx/dy relative to initial screen position
		int xn = xc + dx;
		int yn = yc + dy;
		if (opcode == 3) {
			SDL_RenderDrawLine(renderer, x, y, xn, yn);
		}
		x = xn;
		y = yn;
		data++;
	}

	return SCALE(font->bgi.widths[ch - font->bgi.stroke_hdr.char_start]);
}

int font_bgi_get_char_width(const font_t *font, char ch) {
	return SCALE(font->bgi.widths[ch - font->bgi.stroke_hdr.char_start]);
}

int font_bgi_get_line_height(const font_t *font) {
	return SCALE(font->bgi.stroke_hdr.origin_ascender - font->bgi.stroke_hdr.origin_descender);
}
