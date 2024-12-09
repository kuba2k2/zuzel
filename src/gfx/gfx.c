// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "gfx.h"

void gfx_set_color(SDL_Renderer *renderer, int color) {
	gfx_set_color_alpha(renderer, color, 0xFF);
}

void gfx_set_color_alpha(SDL_Renderer *renderer, int color, int alpha) {
	if (color <= 0b111) {
		SDL_SetRenderDrawColor(
			renderer,
			0xAA * ((color >> 2) & 1),
			0xAA * ((color >> 1) & 1),
			0xAA * ((color >> 0) & 1),
			alpha
		);
	} else {
		color &= 0b111;
		SDL_SetRenderDrawColor(
			renderer,
			0x55 + 0xAA * ((color >> 2) & 1),
			0x55 + 0xAA * ((color >> 1) & 1),
			0x55 + 0xAA * ((color >> 0) & 1),
			alpha
		);
	}
}

void gfx_draw_half_circle(SDL_Renderer *renderer, int cx, int cy, int angle, int radius, int width) {
	SDL_Rect rects[radius * 4];
	const int diameter = (radius * 2);
	int x			   = (radius - 1);
	int y			   = 0;
	int tx			   = 1;
	int ty			   = 1;
	int error		   = (tx - diameter);

	int count = 0;
	while (x >= y) {
		if (angle == 0 || angle == 90) {
			rects[count].x	 = cx - x;
			rects[count++].y = cy - y;
			rects[count].x	 = cx - y;
			rects[count++].y = cy - x;
		}
		if (angle == 0 || angle == 270) {
			rects[count].x	 = cx + x;
			rects[count++].y = cy - y;
			rects[count].x	 = cx + y;
			rects[count++].y = cy - x;
		}
		if (angle == 90 || angle == 180) {
			rects[count].x	 = cx - x;
			rects[count++].y = cy + y;
			rects[count].x	 = cx - y;
			rects[count++].y = cy + x;
		}
		if (angle == 180 || angle == 270) {
			rects[count].x	 = cx + x;
			rects[count++].y = cy + y;
			rects[count].x	 = cx + y;
			rects[count++].y = cy + x;
		}

		if (error <= 0) {
			y++;
			error += ty;
			ty += 2;
		}
		if (error > 0) {
			x--;
			tx += 2;
			error += (tx - diameter);
		}
	}

	int shift = (width - 1) / 2;
	for (int i = 0; i < count; i++) {
		rects[i].x -= shift;
		rects[i].y -= shift;
		rects[i].w = width;
		rects[i].h = width;
	}
	SDL_RenderFillRects(renderer, rects, count);
}
