// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#include "view.h"

static void gfx_view_inflate_frame(view_t *frame, cJSON *json, const view_inflate_on_event_t *on_event);
static void gfx_view_free_frame(view_t *frame);
static void gfx_view_measure_frame(view_t *frame);
static void gfx_view_layout_frame(view_t *frame);
static void gfx_view_draw_frame(SDL_Renderer *renderer, view_t *frame);

view_t *gfx_view_make_frame(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type	  = VIEW_TYPE_FRAME;
	view->inflate = gfx_view_inflate_frame;
	view->free	  = gfx_view_free_frame;
	view->measure = gfx_view_measure_frame;
	view->layout  = gfx_view_layout_frame;
	view->draw	  = gfx_view_draw_frame;
	view->parent  = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_frame(view_t *frame, cJSON *json, const view_inflate_on_event_t *on_event) {
	if (frame->children != NULL)
		LT_ERR(E, return, "Frame children already inflated");
	frame->children = gfx_view_inflate(cJSON_GetObjectItem(json, "children"), frame, on_event);
}

static void gfx_view_free_frame(view_t *frame) {
	if (frame == NULL)
		return;
	gfx_view_free(frame->children);
	free(frame);
}

static void gfx_view_measure_frame(view_t *frame) {
	view_t *child;
	int frame_w = frame->rect.w;
	int frame_h = frame->rect.h;

	int child_max_w = 0;
	int child_max_h = 0;

	DL_FOREACH(frame->children, child) {
		if (child->is_gone)
			continue;
		gfx_view_measure_one(child, frame_w, frame_h);
		child_max_w = max(child_max_w, child->rect.w);
		child_max_h = max(child_max_h, child->rect.h);
	}

	if (frame->rect.w == 0)
		frame->rect.w = child_max_w;
	if (frame->rect.h == 0)
		frame->rect.h = child_max_h;
}

static void gfx_view_layout_frame(view_t *frame) {
	view_t *child;
	int frame_x = frame->rect.x;
	int frame_y = frame->rect.y;
	int frame_w = frame->rect.w;
	int frame_h = frame->rect.h;

	DL_FOREACH(frame->children, child) {
		if (child->is_gone)
			continue;
		gfx_view_layout_one(child, frame_x, frame_y, frame_w, frame_h);
	}
}

static void gfx_view_draw_frame(SDL_Renderer *renderer, view_t *frame) {
	gfx_view_draw(renderer, frame->children);
}
