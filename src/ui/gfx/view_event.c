// Copyright (c) Kuba Szczodrzyński 2024-12-16.

#include "view.h"

bool gfx_view_on_keydown(view_t *views, SDL_Event *e) {
	view_t *focused;
	GFX_VIEW_FIND(views, focused, next, true, focused->is_focused);
	view_t *focusable = NULL;

	if (e->key.keysym.sym == SDLK_DOWN)
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
		if (focused != NULL)
			focused->is_focused = false;
		focusable->is_focused = true;
		return true;
	}
	return false;
}
