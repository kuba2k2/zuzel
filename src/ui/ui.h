// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#pragma once

#include "include.h"

#include "events.h"

typedef struct fragment_t fragment_t;
typedef struct net_t net_t;

typedef enum {
	UI_STATE_MAIN = 0,
	UI_STATE_SERVER_NEW,
	UI_STATE_SERVER_JOIN,
	UI_STATE_BROWSE,
	UI_STATE_ERROR,
	UI_STATE_GAME,
	UI_STATE_MAX,
} ui_state_t;

typedef struct ui_t {
	SDL_Renderer *renderer; //!< SDL renderer
	SDL_Texture *texture;	//!< SDL texture

	ui_state_t state;	   //!< State of the UI
	ui_state_t prev_state; //!< Previous state
	ui_state_t next_state; //!< Next state

	fragment_t *fragments[UI_STATE_MAX];

	net_t *server;
	net_t *client;
} ui_t;

ui_t *ui_init(SDL_Renderer *renderer);
int ui_run(ui_t *ui);
void ui_free(ui_t *ui);

// utils.c
void ui_state_set(ui_t *ui, ui_state_t state);
void ui_state_set_via(ui_t *ui, ui_state_t state, ui_state_t via);
void ui_state_prev(ui_t *ui);
void ui_state_next(ui_t *ui);
