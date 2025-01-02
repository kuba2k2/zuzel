// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#pragma once

#include "include.h"

typedef struct view_t view_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;

typedef struct fragment_t {
	const unsigned char *json;
	view_t *views;
	const view_inflate_on_event_t *inflate_on_event;
} fragment_t;

extern fragment_t fragment_main;
extern fragment_t fragment_server_join;
extern fragment_t fragment_server_new;

bool fragment_init_all(fragment_t **fragments, void *param);
