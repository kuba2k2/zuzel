// Copyright (c) Kuba Szczodrzyński 2025-1-4.

#include "view.h"

static void gfx_view_inflate_rect(view_t *rect, cJSON *json, const view_inflate_on_event_t *on_event);
static void gfx_view_free_rect(view_t *rect);
static void gfx_view_measure_rect(view_t *rect);
static void gfx_view_draw_rect(SDL_Renderer *renderer, view_t *rect);

#define RECT_TRANSPARENT 0x01000000

view_t *gfx_view_make_rect(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type			   = VIEW_TYPE_RECT;
	view->inflate		   = gfx_view_inflate_rect;
	view->free			   = gfx_view_free_rect;
	view->measure		   = gfx_view_measure_rect;
	view->draw			   = gfx_view_draw_rect;
	view->data.rect.fill   = RECT_TRANSPARENT;
	view->data.rect.stroke = RECT_TRANSPARENT;
	view->data.rect.width  = 0;
	view->parent		   = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_rect(view_t *rect, cJSON *json, const view_inflate_on_event_t *on_event) {
	json_read_gfx_color(json, "fill", &rect->data.rect.fill);
	json_read_gfx_color(json, "stroke", &rect->data.rect.stroke);
	json_read_int(json, "width", &rect->data.rect.width);
}

static void gfx_view_free_rect(view_t *rect) {
	if (rect == NULL)
		return;
	free(rect);
}

static void gfx_view_measure_rect(view_t *rect) {
	if (rect->rect.w == 0)
		rect->rect.w = 100;
	if (rect->rect.h == 0)
		rect->rect.h = 100;
}

static void gfx_view_draw_rect(SDL_Renderer *renderer, view_t *rect) {
	int x = rect->rect.x;
	int y = rect->rect.y;
	int w = rect->rect.w;
	int h = rect->rect.h;

	// draw the rectangle
	int width = rect->data.rect.width;
	if (rect->data.rect.fill != RECT_TRANSPARENT) {
		gfx_set_color(renderer, rect->data.rect.fill);
		gfx_draw_rect(renderer, x + width, y + width, w - width * 2, h - width * 2, true);
	}
	if (rect->data.rect.stroke != RECT_TRANSPARENT) {
		gfx_set_color(renderer, rect->data.rect.stroke);
		for (int i = 0; i < width; i++) {
			gfx_draw_rect(renderer, x + i, y + i, w - i * 2, h - i * 2, false);
		}
	}
}
