// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#include "fragment.h"

static bool on_btn_cancel(view_t *view, SDL_Event *e, ui_t *ui) {
	ui_state_prev(ui);
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_cancel),
	GFX_VIEW_ON_EVENT_END(),
};

static bool on_event(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	return false;
}

fragment_t fragment_connecting = {
	.on_event		  = on_event,
	.inflate_on_event = inflate_on_event,
};
