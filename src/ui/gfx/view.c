// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

bool gfx_view_bounding_box = false;

view_t *gfx_view_inflate(cJSON *json, view_t *parent, const view_inflate_on_event_t *on_event) {
	if (json == NULL)
		return NULL;

	if (cJSON_IsArray(json)) {
		view_t *views = NULL;
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			view_t *view = gfx_view_inflate(item, parent, on_event);
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
	else if (strcmp(type->valuestring, "button") == 0)
		view = gfx_view_make_button(parent);
	else if (strcmp(type->valuestring, "slider") == 0)
		view = gfx_view_make_slider(parent);
	else if (strcmp(type->valuestring, "input") == 0)
		view = gfx_view_make_input(parent);
	else if (strcmp(type->valuestring, "rect") == 0)
		view = gfx_view_make_rect(parent);
	else if (strcmp(type->valuestring, "canvas") == 0)
		view = gfx_view_make_canvas(parent);
	else
		LT_ERR(E, return NULL, "Unknown view type '%s'", type->valuestring);

	if (view == NULL)
		LT_ERR(E, return NULL, "Created view '%s' is NULL", type->valuestring);

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

	// read event handlers
	json_read_gfx_view_on_event(json, "on_focus", &view->event.focus, on_event);
	json_read_gfx_view_on_event(json, "on_press", &view->event.press, on_event);
	json_read_gfx_view_on_event(json, "on_change", &view->event.change, on_event);

	if (view->inflate != NULL)
		view->inflate(view, json, on_event);
	else
		LT_W("View '%s' does not provide 'inflate' function", view->id);

	return view;
}

view_t *gfx_view_clone(view_t *view, view_t *parent) {
	view_t *clone;
	MALLOC(clone, sizeof(*clone), return NULL);
	memcpy(clone, view, sizeof(*clone));

	// duplicate the ID
	if (view->id != NULL)
		clone->id = strdup(view->id);
	// update view-owned memory
	clone->children = NULL;
	clone->parent	= parent;
	clone->prev		= NULL;
	clone->next		= NULL;

	// clone all children
	view_t *child;
	DL_FOREACH(view->children, child) {
		view_t *child_clone = gfx_view_clone(child, clone);
		if (child_clone == NULL)
			goto fail;
		DL_APPEND(clone->children, child_clone);
	}

	// clone view-specific data
	if (clone->clone != NULL)
		clone->clone(view, clone);

	return clone;

fail:
	gfx_view_free(clone);
	return NULL;
}

void gfx_view_free(view_t *views) {
	view_t *view, *tmp;
	DL_FOREACH_SAFE(views, view, tmp) {
		DL_DELETE(views, view);
		if (view->free != NULL) {
			free(view->id);
			view->free(view);
		} else {
			LT_W("View '%s' does not provide 'free' function", view->id);
			free(view->id);
			free(view);
		}
	}
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
		if (gfx_view_bounding_box) {
			gfx_set_color(renderer, 0x7FFF0000);
			gfx_draw_rect(renderer, view->rect.x, view->rect.y, view->rect.w, view->rect.h, false);
		}
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
				focused->event.focus(focused, e, focused->event.param);
		}
		focusable->is_focused = true;
		if (focusable->event.focus != NULL)
			focusable->event.focus(focusable, e, focusable->event.param);
		return true;
	}
	return false;
}

static bool gfx_view_on_mouse_motion(view_t *views, view_t *focused, SDL_Event *e) {
	int x = e->motion.x / SETTINGS->screen.scale;
	int y = e->motion.y / SETTINGS->screen.scale;

	view_t *focusable = NULL;
	view_t *view	  = views;
	while (view != NULL) {
		if (GFX_VIEW_IS_ACTIVE(view) && GFX_VIEW_IN_BOX(view, x, y))
			focusable = view;
		view = gfx_view_find_next(view);
	}

	// only send events if:
	// - the new focused view is different from the previous one
	// - the new focused view is the same, but not focused
	//   (happens if the view uses 'in_event = true')
	if (focusable != focused || (focusable != NULL && !focusable->is_focused)) {
		bool ret = false;
		if (focused != NULL && focused->is_focused == true) {
			focused->is_focused = false;
			if (focused->event.focus != NULL)
				focused->event.focus(focused, e, focused->event.param);
			ret = true;
		}
		if (focusable != NULL && focusable->is_focused == false) {
			focusable->is_focused = true;
			if (focusable->event.focus != NULL)
				focusable->event.focus(focusable, e, focusable->event.param);
			ret = true;
		}
		return ret;
	}
	return false;
}

bool gfx_view_on_event(view_t *views, SDL_Event *e) {
	bool ret = false;

	view_t *in_event;
	GFX_VIEW_FIND(views, in_event, next, true, in_event->in_event);
	view_t *focused;
	GFX_VIEW_FIND(views, focused, next, true, focused->is_focused);

	if (in_event != NULL && in_event->on_event)
		ret = in_event->on_event(in_event, e, in_event->event.param);

	switch (e->type) {
		case SDL_KEYDOWN:
			ret = ret || gfx_view_on_key_down(views, focused, e);
			break;

		case SDL_MOUSEMOTION:
			ret = gfx_view_on_mouse_motion(views, focused, e) || ret;
			break;
	}

	if (focused != NULL && focused->on_event && !ret)
		ret = focused->on_event(focused, e, focused->event.param);

	return ret;
}
