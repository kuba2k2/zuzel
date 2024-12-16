// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

void gfx_view_measure_one(view_t *view, int parent_w, int parent_h) {
	// set rect width based on the spec
	if (view->w == VIEW_WRAP_CONTENT)
		view->rect.w = 0;
	else if (view->w != VIEW_MATCH_PARENT)
		view->rect.w = view->w;
	else if (parent_w != 0)
		view->rect.w = parent_w;
	else {
		LT_W("View '%s' has unknown parent width", view->id);
		view->rect.w = 0;
	}

	// set rect height based on the spec
	if (view->h == VIEW_WRAP_CONTENT)
		view->rect.h = 0;
	else if (view->h != VIEW_MATCH_PARENT)
		view->rect.h = view->h;
	else if (parent_h != 0)
		view->rect.h = parent_h;
	else {
		LT_W("View '%s' has unknown parent height", view->id);
		view->rect.h = 0;
	}

	// let the view calculate its width/height
	if (view->measure != NULL)
		view->measure(view);
	else
		LT_W("View '%s' does not provide 'measure' function", view->id);

	// add margins
	view->rect.w += view->ml + view->mr;
	view->rect.h += view->mt + view->mb;
}

void gfx_view_layout_one(view_t *view, int x, int y, int parent_w, int parent_h) {
	// align horizontally
	if (view->gravity & GFX_ALIGN_RIGHT)
		view->rect.x = parent_w - view->rect.w;
	else if (view->gravity & GFX_ALIGN_CENTER_HORIZONTAL)
		view->rect.x = (parent_w - view->rect.w) / 2;
	else
		view->rect.x = 0;

	// align vertically
	if (view->gravity & GFX_ALIGN_BOTTOM)
		view->rect.y = parent_h - view->rect.h;
	else if (view->gravity & GFX_ALIGN_CENTER_VERTICAL)
		view->rect.y = (parent_h - view->rect.h) / 2;
	else
		view->rect.y = 0;

	// add parent view's offset
	view->rect.x += x;
	view->rect.y += y;

	// add margins
	view->rect.x += view->ml;
	view->rect.y += view->mt;
	view->rect.w -= view->ml + view->mr;
	view->rect.h -= view->mt + view->mb;

	// let the view reposition its children, knowing its final X/Y position
	if (view->layout != NULL)
		view->layout(view);
}

void gfx_view_set_text_style(view_text_t *text, unsigned int color, int font, int size, int align) {
	text->color = color;
	text->font	= font;
	text->size	= size;
	text->align = align;
}
