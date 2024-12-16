// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

view_t *gfx_view_inflate(cJSON *json, view_t *parent) {
	if (json == NULL)
		return NULL;

	if (cJSON_IsArray(json)) {
		view_t *views = NULL;
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			view_t *view = gfx_view_inflate(item, parent);
			if (view != NULL)
				DL_APPEND(views, view);
		}
		return views;
	}
	if (!cJSON_IsObject(json))
		LT_ERR(E, return NULL, "View JSON is not an object");

	view_t *view = NULL;
	cJSON *type	 = cJSON_GetObjectItem(json, "type");

	if (strcmp(type->valuestring, "frame") == 0)
		view = gfx_view_make_frame(parent);
	else if (strcmp(type->valuestring, "box") == 0)
		view = gfx_view_make_box(parent);
	else if (strcmp(type->valuestring, "text") == 0)
		view = gfx_view_make_text(parent);
	else
		LT_ERR(E, return NULL, "Unknown view type '%s'", type->valuestring);

	if (view == NULL)
		LT_ERR(E, return NULL, "Created view '%s' is NULL", type->valuestring);

	view->parent = parent;
	json_read_string(json, "id", &view->id);
	json_read_gfx_size(json, "w", &view->w);
	json_read_gfx_size(json, "h", &view->h);
	json_read_gfx_align(json, "gravity", &view->gravity);
	json_read_int(json, "weight", &view->weight);
	json_read_bool(json, "is_gone", &view->is_gone);
	json_read_bool(json, "is_invisible", &view->is_invisible);
	json_read_bool(json, "is_disabled", &view->is_disabled);
	json_read_bool(json, "is_focusable", &view->is_focusable);

	// read margins (m: all, mh: horizontal, mv: vertical)
	json_read_int(json, "m", &view->ml);
	view->mt = view->ml;
	json_read_int(json, "mh", &view->ml);
	view->mr = view->ml;
	json_read_int(json, "mv", &view->mt);
	view->mb = view->mt;
	json_read_int(json, "ml", &view->ml);
	json_read_int(json, "mr", &view->mr);
	json_read_int(json, "mt", &view->mt);
	json_read_int(json, "mb", &view->mb);

	if (view->inflate != NULL)
		view->inflate(view, json);
	else
		LT_W("View '%s' does not provide 'inflate' function", view->id);

	if (parent == NULL) {
		// set default focus to the first (focusable) view
		view_t *focusable;
		GFX_VIEW_FIND(view, focusable, next, true, GFX_VIEW_IS_ACTIVE(focusable));
		if (focusable != NULL)
			focusable->is_focused = true;
	}

	return view;
}

void gfx_view_measure(view_t *views) {
	int screen_w = SETTINGS->screen.width;
	int screen_h = SETTINGS->screen.height;

	view_t *view;
	DL_FOREACH(views, view) {
		if (view->is_gone)
			continue;
		gfx_view_measure_one(view, screen_w, screen_h);
	}
}

void gfx_view_layout(view_t *views) {
	int screen_w = SETTINGS->screen.width;
	int screen_h = SETTINGS->screen.height;

	view_t *view;
	DL_FOREACH(views, view) {
		if (view->is_gone)
			continue;
		gfx_view_layout_one(view, 0, 0, screen_w, screen_h);
	}
}

void gfx_view_draw(SDL_Renderer *renderer, view_t *views) {
	view_t *view;
	DL_FOREACH(views, view) {
		if (view->is_gone || view->is_invisible)
			continue;
		// call the view's rendering function
		if (view->draw != NULL)
			view->draw(renderer, view);
		else
			LT_W("View '%s' does not provide 'draw' function", view->id);
		// draw the bounding box
		gfx_set_color(renderer, 0x7FFF0000);
		gfx_draw_rect(renderer, view->rect.x, view->rect.y, view->rect.w, view->rect.h, false);
	}
}

bool gfx_view_on_keydown(view_t *views, SDL_Event *e);

bool gfx_view_on_event(view_t *views, SDL_Event *e) {
	bool ret;
	switch (e->type) {
		case SDL_KEYDOWN:
			ret = gfx_view_on_keydown(views, e);
			LT_D("Event SDL_KEYDOWN = %d", ret);
			return ret;
	}
	return false;
}
