// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

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
