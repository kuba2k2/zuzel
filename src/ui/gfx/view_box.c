// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

static void gfx_view_measure_box(view_t *box);
static void gfx_view_layout_box(view_t *box);
static void gfx_view_draw_box(SDL_Renderer *renderer, view_t *box);

view_t *gfx_view_make_box(bool is_horizontal) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type					 = VIEW_TYPE_BOX;
	view->measure				 = gfx_view_measure_box;
	view->layout				 = gfx_view_layout_box;
	view->draw					 = gfx_view_draw_box;
	view->data.box.is_horizontal = is_horizontal;

	return view;
}

static void gfx_view_measure_box(view_t *box) {
	view_t *child;
	bool is_horizontal = box->data.box.is_horizontal;

	int box_size = is_horizontal ? box->rect.w : box->rect.h;

	int child_size_max	 = 0;
	int child_size_known = 0;

	int weight_sum = 0;
	LL_FOREACH(box->data.box.children, child) {
		if (child->is_gone)
			continue;
		if ((is_horizontal ? child->w : child->h) != VIEW_MATCH_PARENT || box_size == 0) {
			// measure views with known size (specified in pixels or view's own size)
			// also measure if the box's size is not known (not possible to use weighted views then)
			child->weight = 0; // reset to 0 to ignore this view later
			// measure the view (set the parent size to 0, because it doesn't matter)
			if (is_horizontal)
				gfx_view_measure_one(child, 0, box->rect.h);
			else
				gfx_view_measure_one(child, box->rect.w, 0);
			// count the total known size of child views, in layout direction
			child_size_known += is_horizontal ? child->rect.w : child->rect.h;
		} else {
			// coerce weight to at least 1
			if (child->weight == 0)
				child->weight = 1;
			weight_sum += child->weight;
		}
		// save maximum known size in the non-layout direction
		if (is_horizontal)
			child_size_max = max(child_size_max, child->rect.h);
		else
			child_size_max = max(child_size_max, child->rect.w);
	}

	// only recalculate weighted child views if there is anything to recalculate
	if (weight_sum != 0) {
		int free_size = box_size - child_size_known;
		LL_FOREACH(box->data.box.children, child) {
			if (child->is_gone)
				continue;
			if ((is_horizontal ? child->w : child->h) != VIEW_MATCH_PARENT)
				continue;
			if (is_horizontal) {
				child->rect.w = child->weight * free_size / weight_sum;
				gfx_view_measure_one(child, child->rect.w, box->rect.h);
			} else {
				child->rect.h = child->weight * free_size / weight_sum;
				gfx_view_measure_one(child, box->rect.w, child->rect.h);
			}
		}
	}

	if (box->rect.w == 0)
		box->rect.w = is_horizontal ? child_size_known : child_size_max;
	if (box->rect.h == 0)
		box->rect.h = is_horizontal ? child_size_max : child_size_known;
}

static void gfx_view_layout_box(view_t *box) {
	view_t *child;
	bool is_horizontal = box->data.box.is_horizontal;

	int box_x = box->rect.x;
	int box_y = box->rect.y;
	int box_w = box->rect.w;
	int box_h = box->rect.h;
	int pos	  = 0;

	LL_FOREACH(box->data.box.children, child) {
		if (child->is_gone)
			return;
		if (is_horizontal) {
			gfx_view_layout_one(child, box_x + pos, box_y, box_w - pos, box_h);
			pos += child->rect.w;
		} else {
			gfx_view_layout_one(child, box_x, box_y + pos, box_w, box_h - pos);
			pos += child->rect.h;
		}
	}
}

static void gfx_view_draw_box(SDL_Renderer *renderer, view_t *box) {
	gfx_view_draw(renderer, box->data.box.children);
}
