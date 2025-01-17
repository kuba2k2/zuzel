// Copyright (c) Kuba Szczodrzyński 2025-1-2.

#pragma once

#include "include.h"

typedef struct view_t view_t;
typedef struct view_inflate_on_event_t view_inflate_on_event_t;
typedef struct ui_t ui_t;
typedef struct fragment_t fragment_t;
typedef bool (*fragment_on_event_t)(ui_t *ui, fragment_t *fragment, SDL_Event *e);
typedef void (*dialog_cb_t)(ui_t *ui, int dialog_id, const char *value);

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

// fragment.c
bool fragment_init_all(fragment_t **fragments, void *param);
void fragment_reload(fragment_t *fragment, void *param);

// dialog.c
bool dialog_init(ui_t *ui, view_t *views);
void dialog_hide(ui_t *ui);
void dialog_show_edit(ui_t *ui, int id, dialog_cb_t cb, const char *title, const char *value, int max_length);
void dialog_show_prompt(ui_t *ui, int id, dialog_cb_t cb, const char *title, const char *message);
bool on_dialog_edit_input(view_t *view, SDL_Event *e, ui_t *ui);
bool on_dialog_edit_ok(view_t *view, SDL_Event *e, ui_t *ui);
bool on_dialog_prompt_yes(view_t *view, SDL_Event *e, ui_t *ui);
bool on_dialog_prompt_no(view_t *view, SDL_Event *e, ui_t *ui);
