// Copyright (c) Kuba Szczodrzyński 2025-1-3.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	char *message = lt_log_get_errors("");
	gfx_view_set_text(gfx_view_find_by_id(fragment->views, "message"), message);
	free(message);
	return true;
}

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_prev(ui);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_error = {
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
