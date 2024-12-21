// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

view_t *gfx_view_inflate_from_file(const char *filename, view_t *parent, const view_inflate_on_event_t *on_event) {
	cJSON *json	  = file_read_json(filename);
	view_t *views = gfx_view_inflate(json, parent, on_event);
	cJSON_Delete(json);
	return views;
}

void gfx_view_measure_one(view_t *view, int parent_w, int parent_h) {
	// set rect width based on the spec
	if (view->w == VIEW_WRAP_CONTENT)
		view->rect.w = 0;
	else if (view->w != VIEW_MATCH_PARENT)
		view->rect.w = view->w;
	else if (parent_w != 0)
		view->rect.w = parent_w - view->ml - view->mr;
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
		view->rect.h = parent_h - view->mt - view->mb;
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

view_t *gfx_view_find_prev(view_t *view) {
	if (view->prev != NULL && view->prev->next == view)
		view = view->prev;
	else if (view->parent != NULL && view->parent->prev != NULL && view->parent->prev->next == view->parent)
		view = view->parent->prev;
	else
		view = NULL;
	if (view != NULL && view->children != NULL)
		view = view->children->prev;
	return view;
}

view_t *gfx_view_find_next(view_t *view) {
	if (view->children != NULL)
		return view->children;
	else if (view->next != NULL)
		return view->next;
	else if (view->parent != NULL)
		return view->parent->next;
	else
		return NULL;
}

view_t *gfx_view_find_by_id(view_t *views, const char *id) {
	while (views != NULL && (views->id == NULL || strcmp(views->id, id) != 0)) {
		views = gfx_view_find_next(views);
	}
	return views;
}

char *gfx_view_make_id(view_t *view) {
	const char *type_str = "view";
	switch (view->type) {
		case VIEW_TYPE_FRAME:
			type_str = "frame";
			break;
		case VIEW_TYPE_BOX:
			type_str = "box";
			break;
		case VIEW_TYPE_TEXT:
			type_str = "text";
			break;
		case VIEW_TYPE_BUTTON:
			type_str = "button";
			break;
		case VIEW_TYPE_SLIDER:
			type_str = "slider";
			break;
		case VIEW_TYPE_INPUT:
			type_str = "input";
			break;
	}

	static int counter = 0;
	char buf[32];
	sprintf(buf, "%s%03d", type_str, ++counter);
	return strdup(buf);
}

void gfx_view_set_event_param(view_t *views, const char *id, void *param) {
	if (id == NULL) {
		while (views != NULL) {
			views->event.param = param;
			views			   = gfx_view_find_next(views);
		}
	} else {
		view_t *view = gfx_view_find_by_id(views, id);
		if (view != NULL)
			view->event.param = param;
	}
}
