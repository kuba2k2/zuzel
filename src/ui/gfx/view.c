// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

view_t *gfx_view_inflate(cJSON *json) {
	if (json == NULL)
		return NULL;

	if (cJSON_IsArray(json)) {
		view_t *views = NULL;
		cJSON *item;
		cJSON_ArrayForEach(item, json) {
			view_t *view = gfx_view_inflate(item);
			if (view != NULL)
				LL_APPEND(views, view);
		}
		return views;
	}
	if (!cJSON_IsObject(json))
		LT_ERR(E, return NULL, "View JSON is not an object");

	view_t *view = NULL;
	cJSON *type	 = cJSON_GetObjectItem(json, "type");

	if (strcmp(type->valuestring, "box") == 0)
		view = gfx_view_make_box();
	else if (strcmp(type->valuestring, "text") == 0)
		view = gfx_view_make_text();
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

	if (view->inflate != NULL)
		view->inflate(view, json);
	else
		LT_W("View '%s' does not provide 'inflate' function", view->id);

	return view;
}

void gfx_view_measure(view_t *views) {
	int screen_w = SETTINGS->screen.width;
	int screen_h = SETTINGS->screen.height;

	view_t *view;
	LL_FOREACH(views, view) {
		if (view->is_gone)
			continue;
		gfx_view_measure_one(view, screen_w, screen_h);
	}
}

void gfx_view_layout(view_t *views) {
	int screen_w = SETTINGS->screen.width;
	int screen_h = SETTINGS->screen.height;

	view_t *view;
	LL_FOREACH(views, view) {
		if (view->is_gone)
			continue;
		gfx_view_layout_one(view, 0, 0, screen_w, screen_h);
	}
}

void gfx_view_draw(SDL_Renderer *renderer, view_t *views) {
	view_t *view;
	LL_FOREACH(views, view) {
		if (view->is_gone || view->is_invisible)
			continue;
		// call the view's rendering function
		if (view->draw != NULL)
			view->draw(renderer, view);
		else
			LT_W("View '%s' does not provide 'draw' function", view->id);
		// draw the bounding box
		gfx_set_color(renderer, 0xFF0000);
		SDL_RenderDrawRect(renderer, &view->rect);
	}
}
