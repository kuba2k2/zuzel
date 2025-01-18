// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "view.h"

static void gfx_view_inflate_canvas(view_t *canvas, cJSON *json, const view_inflate_on_event_t *on_event);
static void gfx_view_free_canvas(view_t *canvas);
static void gfx_view_measure_canvas(view_t *canvas);
static void gfx_view_draw_canvas(SDL_Renderer *renderer, view_t *canvas);

view_t *gfx_view_make_canvas(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type	  = VIEW_TYPE_CANVAS;
	view->inflate = gfx_view_inflate_canvas;
	view->free	  = gfx_view_free_canvas;
	view->measure = gfx_view_measure_canvas;
	view->draw	  = gfx_view_draw_canvas;
	view->parent  = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_canvas(view_t *canvas, cJSON *json, const view_inflate_on_event_t *on_event) {
	// nothing to inflate
}

static void gfx_view_free_canvas(view_t *canvas) {
	if (canvas == NULL)
		return;
	free(canvas);
}

static void gfx_view_measure_canvas(view_t *canvas) {
	if (canvas->rect.w == 0)
		canvas->rect.w = 100;
	if (canvas->rect.h == 0)
		canvas->rect.h = 100;
}

static void gfx_view_draw_canvas(SDL_Renderer *renderer, view_t *canvas) {
	// nothing by default
	LT_W("Canvas does not provide draw implementation");
}
