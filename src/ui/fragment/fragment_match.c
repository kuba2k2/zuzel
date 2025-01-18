// Copyright (c) Kuba Szczodrzyński 2025-1-18.

#include "fragment.h"

static bool on_show(ui_t *ui, fragment_t *fragment, SDL_Event *e) {
	return true;
}

static const view_inflate_on_event_t inflate_on_event[] = {
	GFX_VIEW_ON_EVENT_END(),
};

fragment_t fragment_match = {
	.on_show		  = on_show,
	.inflate_on_event = inflate_on_event,
};
