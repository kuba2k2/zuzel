// Copyright (c) Kuba Szczodrzyński 2024-12-15.

#include "view.h"

static void gfx_view_inflate_text(view_t *text, cJSON *json, const view_inflate_on_event_t *on_event);
static void gfx_view_free_text(view_t *text);
static void gfx_view_measure_text(view_t *text);
static void gfx_view_draw_text(SDL_Renderer *renderer, view_t *text);

view_t *gfx_view_make_text(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type			  = VIEW_TYPE_TEXT;
	view->inflate		  = gfx_view_inflate_text;
	view->free			  = gfx_view_free_text;
	view->measure		  = gfx_view_measure_text;
	view->draw			  = gfx_view_draw_text;
	view->data.text.color = GFX_COLOR_BRIGHT_WHITE;
	view->data.text.size  = FONT_SIZE_DEFAULT;
	view->data.text.align = GFX_ALIGN_DEFAULT;
	view->parent		  = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_text(view_t *text, cJSON *json, const view_inflate_on_event_t *on_event) {
	json_read_gfx_view_text(json, "text", &text->data.text);
}

static void gfx_view_free_text(view_t *text) {
	if (text == NULL)
		return;
	free(text->data.text.text);
	free(text);
}

static void gfx_view_measure_text(view_t *text) {
	gfx_set_text_style(text->data.text.font, text->data.text.size, text->data.text.align);
	if (text->rect.w == 0)
		text->rect.w = gfx_get_text_width(text->data.text.text, false);
	if (text->rect.h == 0)
		text->rect.h = gfx_get_text_height(text->data.text.text);
}

static void gfx_view_draw_text(SDL_Renderer *renderer, view_t *text) {
	int x = text->rect.x;
	int y = text->rect.y;

	if (text->data.text.align & GFX_ALIGN_CENTER_HORIZONTAL)
		x += text->rect.w / 2;
	else if (text->data.text.align & GFX_ALIGN_RIGHT)
		x += text->rect.w;

	if (text->data.text.align & GFX_ALIGN_CENTER_VERTICAL)
		y += text->rect.h / 2;
	else if (text->data.text.align & GFX_ALIGN_BOTTOM)
		y += text->rect.h;

	gfx_set_color(renderer, text->data.text.color);
	gfx_set_text_style(text->data.text.font, text->data.text.size, text->data.text.align);
	gfx_draw_text(renderer, x, y, text->data.text.text);
}
