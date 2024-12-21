// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

static bool on_btn_back(view_t *view, SDL_Event *e, ui_t *menu) {
	menu->state = UI_STATE_MENU_MAIN;
	return true;
}

static bool on_btn_local(view_t *view, SDL_Event *e, ui_t *menu) {
	if (menu->server == NULL)
		menu->server = net_server_start();
	if (menu->server == NULL)
		LT_ERR(F, goto cleanup, "Couldn't start the server");

	if (menu->client == NULL)
		menu->client = net_client_start(NULL);
	if (menu->client == NULL)
		LT_ERR(F, goto cleanup, "Couldn't start the client");

	return true;

cleanup:
	net_stop(menu->server);
	net_stop(menu->client);
	menu->server = NULL;
	menu->client = NULL;
	return true;
}

const view_inflate_on_event_t menu_server_new_on_event[] = {
	GFX_VIEW_ON_EVENT(on_btn_back),
	GFX_VIEW_ON_EVENT(on_btn_local),
	GFX_VIEW_ON_EVENT_END(),
};
