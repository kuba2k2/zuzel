// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#pragma once

#include "include.h"

typedef struct view_t view_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;
typedef struct ui_t ui_t;
typedef struct fragment_t fragment_t;
typedef bool (*fragment_on_event_t)(ui_t *ui, fragment_t *fragment, SDL_Event *e);

typedef struct fragment_t {
	const char *file;
	const char *json;
	view_t *views;
	const fragment_on_event_t on_show;
	const fragment_on_event_t on_hide;
	const fragment_on_event_t on_event;
	const view_inflate_on_event_t *inflate_on_event;
} fragment_t;

extern fragment_t fragment_main;
extern fragment_t fragment_server_join;
extern fragment_t fragment_server_new;
extern fragment_t fragment_connecting;
extern fragment_t fragment_browse;
extern fragment_t fragment_lobby;
extern fragment_t fragment_error;

bool fragment_init_all(fragment_t **fragments, void *param);
void fragment_reload(fragment_t *fragment, void *param);
