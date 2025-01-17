// Copyright (c) Kuba Szczodrzyński 2025-1-17.

#include "fragment.h"

static view_t *dialog_bg			 = NULL;
static view_t *dialog_edit			 = NULL;
static view_t *dialog_edit_title	 = NULL;
static view_t *dialog_edit_input	 = NULL;
static view_t *dialog_edit_ok		 = NULL;
static view_t *dialog_prompt		 = NULL;
static view_t *dialog_prompt_title	 = NULL;
static view_t *dialog_prompt_message = NULL;
static view_t *dialog_prompt_yes	 = NULL;
static view_t *dialog_prompt_no		 = NULL;

static int dialog_id		 = 0;
static dialog_cb_t dialog_cb = 0;

bool dialog_init(ui_t *ui, view_t *views) {
	GFX_VIEW_BIND(views, dialog_bg, return false);
	GFX_VIEW_BIND(views, dialog_edit, return false);
	GFX_VIEW_BIND(views, dialog_edit_title, return false);
	GFX_VIEW_BIND(views, dialog_edit_input, return false);
	GFX_VIEW_BIND(views, dialog_edit_ok, return false);
	GFX_VIEW_BIND(views, dialog_prompt, return false);
	GFX_VIEW_BIND(views, dialog_prompt_title, return false);
	GFX_VIEW_BIND(views, dialog_prompt_message, return false);
	GFX_VIEW_BIND(views, dialog_prompt_yes, return false);
	GFX_VIEW_BIND(views, dialog_prompt_no, return false);

	dialog_hide(ui);
	return true;
}

static void set_is_gone(view_t *views, bool is_gone) {
	for (view_t *view = views; view != NULL; view = gfx_view_find_next(view)) {
		view->is_gone = is_gone;
	}
}

void dialog_hide(ui_t *ui) {
	dialog_bg->is_gone	   = true;
	dialog_edit->is_gone   = true;
	dialog_prompt->is_gone = true;
	set_is_gone(dialog_bg, true);
	dialog_id		 = 0;
	dialog_cb		 = NULL;
	ui->force_layout = true;
}

void dialog_show_edit(ui_t *ui, int id, dialog_cb_t cb, const char *title, const char *value, int max_length) {
	dialog_id = id;
	dialog_cb = cb;
	set_is_gone(dialog_bg, false);
	set_is_gone(dialog_prompt, true);
	gfx_view_set_text(dialog_edit_title, title);
	// set input value
	free(dialog_edit_input->data.input.value);
	MALLOC(dialog_edit_input->data.input.value, max_length + 1, return);
	strncpy2(dialog_edit_input->data.input.value, value, max_length);
	dialog_edit_input->data.input.max_length = max_length;
	dialog_edit_input->data.input.pos		 = strlen(dialog_edit_input->data.input.value);
	ui->force_layout						 = true;
}

void dialog_show_prompt(ui_t *ui, int id, dialog_cb_t cb, const char *title, const char *message) {
	dialog_id = id;
	dialog_cb = cb;
	set_is_gone(dialog_bg, false);
	set_is_gone(dialog_edit, true);
	set_is_gone(dialog_prompt, false);
	gfx_view_set_text(dialog_prompt_title, title);
	gfx_view_set_text(dialog_prompt_message, message);
	ui->force_layout = true;
}

bool on_dialog_edit_input(view_t *view, SDL_Event *e, ui_t *ui) {
	dialog_edit_ok->is_disabled = strlen(view->data.input.value) == 0;
	return true;
}

bool on_dialog_edit_ok(view_t *view, SDL_Event *e, ui_t *ui) {
	if (dialog_cb != NULL)
		dialog_cb(ui, dialog_id, dialog_edit_input->data.input.value);
	return true;
}

bool on_dialog_prompt_yes(view_t *view, SDL_Event *e, ui_t *ui) {
	if (dialog_cb != NULL)
		dialog_cb(ui, dialog_id, dialog_prompt_yes->data.button.text.text);
	return true;
}

bool on_dialog_prompt_no(view_t *view, SDL_Event *e, ui_t *ui) {
	dialog_hide(ui);
	return true;
}
