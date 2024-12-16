// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "gfx.h"

/**
 * Change the rendering color.
 *
 * @param renderer SDL renderer
 * @param color color in ARGB8888 format
 */
void gfx_set_color(SDL_Renderer *renderer, unsigned int color) {
	unsigned int a = (color >> 24) & 0xFF;
	unsigned int r = (color >> 16) & 0xFF;
	unsigned int g = (color >> 8) & 0xFF;
	unsigned int b = (color >> 0) & 0xFF;
	if (a == 0x00)
		a = 0xFF;
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

/**
 * Draw points of the specified size.
 *
 * @param renderer SDL renderer
 * @param rects SDL_Rect array pointer with populated x,y coords
 * @param count count of points in the array
 * @param width point width (thickness) in pixels
 */
void gfx_draw_rect_points(SDL_Renderer *renderer, SDL_Rect *rects, int count, int width) {
	int shift = (width - 1) / 2;
	for (int i = 0; i < count; i++) {
		rects[i].x -= shift;
		rects[i].y -= shift;
		rects[i].w = width;
		rects[i].h = width;
	}
	SDL_RenderFillRects(renderer, rects, count);
}

/**
 * Draw a half-circle (two octants) using the midpoint algorithm.
 *
 * @param renderer SDL renderer
 * @param cx center X
 * @param cy center Y
 * @param angle starting angle (0: right, 90: up, 180: left, 270: down)
 * @param radius radius in pixels
 * @param width line thickness in pixels
 */
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

	gfx_draw_rect_points(renderer, rects, count, width);
}

/**
 * Draw a straight line using the Bresenham's algorithm.
 *
 * @param renderer SDL renderer
 * @param x1 start X
 * @param y1 start Y
 * @param x2 end X
 * @param y2 end Y
 * @param width line thickness in pixels
 */
void gfx_draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int width) {
	// calculate line length (+1 to be on the safe side)
	int length = (int)round(sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))) + 1;
	SDL_Rect rects[length];

	// https://gist.github.com/bert/1085538#file-plot_line-c
	int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = dx + dy, e2;

	int count = 0;
	while (1) {
		rects[count].x	 = x1;
		rects[count++].y = y1;
		if (x1 == x2 && y1 == y2)
			break;
		e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x1 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y1 += sy;
		}
	}

	gfx_draw_rect_points(renderer, rects, count, width);
}

void gfx_draw_rect(SDL_Renderer *renderer, int x, int y, int w, int h, bool fill) {
	SDL_Rect rect = {.x = x, .y = y, .w = w, .h = h};
	if (fill)
		SDL_RenderFillRect(renderer, &rect);
	else
		SDL_RenderDrawRect(renderer, &rect);
}
