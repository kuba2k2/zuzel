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

static bool gfx_view_on_key_down(view_t *views, view_t *focused, SDL_Event *e) {
	view_t *focusable = NULL;
	if (e->key.keysym.sym == SDLK_DOWN || e->key.keysym.sym == SDLK_TAB)
		GFX_VIEW_FIND(
			focused == NULL ? views : focused,
			focusable,
			next,
			// can be same if nothing is focused
			focused == NULL,
			GFX_VIEW_IS_ACTIVE(focusable)
		);
	else if (e->key.keysym.sym == SDLK_UP)
		GFX_VIEW_FIND(
			focused == NULL ? views : focused,
			focusable,
			prev,
			// can be same if nothing is focused
			focused == NULL,
			GFX_VIEW_IS_ACTIVE(focusable)
		);
	if (focusable != NULL) {
		if (focused != NULL) {
			focused->is_focused = false;
			if (focused->event.focus != NULL)
				focused->event.focus(focused, e);
		}
		focusable->is_focused = true;
		if (focusable->event.focus != NULL)
			focusable->event.focus(focusable, e);
		return true;
	}
	return false;
}

static bool gfx_view_on_mouse_motion(view_t *views, view_t *focused, SDL_Event *e) {
	int x = e->motion.x;
	int y = e->motion.y;

	view_t *focusable = NULL;
	view_t *view	  = views;
	while (view != NULL) {
		if (GFX_VIEW_IS_ACTIVE(view) && GFX_VIEW_IN_BOX(view, x, y))
			focusable = view;
		view = gfx_view_find_next(view);
	}

	if (focusable != focused) {
		if (focused != NULL) {
			focused->is_focused = false;
			if (focused->event.focus != NULL)
				focused->event.focus(focused, e);
		}
		if (focusable != NULL) {
			focusable->is_focused = true;
			if (focusable->event.focus != NULL)
				focusable->event.focus(focusable, e);
		}
		return true;
	}
	return false;
}

bool gfx_view_on_event(view_t *views, SDL_Event *e) {
	view_t *focused;
	GFX_VIEW_FIND(views, focused, next, true, focused->is_focused);

	bool ret = false;
	switch (e->type) {
		case SDL_KEYDOWN:
			ret = gfx_view_on_key_down(views, focused, e);
			LT_D("Event SDL_KEYDOWN(...) = %d", ret);
			break;
		case SDL_MOUSEMOTION:
			ret = gfx_view_on_mouse_motion(views, focused, e);
			LT_D("Event SDL_MOUSEMOTION(x=%d, y=%d) = %d", e->motion.x, e->motion.y, ret);
			break;
	}

	if (!ret && focused != NULL && focused->on_event)
		ret = focused->on_event(focused, e);

	return ret;
}
