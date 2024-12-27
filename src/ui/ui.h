// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "events.h"

typedef struct view_t view_t;
typedef struct net_t net_t;

typedef enum {
	UI_STATE_MENU_MAIN = 0,
	UI_STATE_MENU_SERVER_NEW,
	UI_STATE_MENU_SERVER_JOIN,
	UI_STATE_MENU_BROWSE,
	UI_STATE_ERROR,
	UI_STATE_GAME,
	UI_STATE_MAX,
} ui_state_t;

typedef struct ui_t {
	SDL_Renderer *renderer; //!< SDL renderer
	SDL_Texture *texture;	//!< SDL texture

	ui_state_t state; //!< State of the UI

	// inflated views
	view_t *views[UI_STATE_MAX];

	net_t *server;
	net_t *client;
} ui_t;

ui_t *ui_init(SDL_Renderer *renderer);
int ui_run(ui_t *ui);
void ui_free(ui_t *ui);
