// Copyright (c) Kuba Szczodrzyński 2024-12-19.

#include "view.h"

static void gfx_view_inflate_slider(view_t *slider, cJSON *json);
static void gfx_view_free_slider(view_t *slider);
static void gfx_view_measure_slider(view_t *slider);
static void gfx_view_draw_slider(SDL_Renderer *renderer, view_t *slider);
static bool gfx_view_on_event_slider(view_t *slider, SDL_Event *e);

view_t *gfx_view_make_slider(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type					 = VIEW_TYPE_SLIDER;
	view->inflate				 = gfx_view_inflate_slider;
	view->free					 = gfx_view_free_slider;
	view->measure				 = gfx_view_measure_slider;
	view->draw					 = gfx_view_draw_slider;
	view->on_event				 = gfx_view_on_event_slider;
	view->is_focusable			 = true;
	view->data.slider.text.color = 0xE0E0E0;
	view->data.slider.text.size	 = FONT_SIZE_DEFAULT;
	view->data.slider.value		 = 1;
	view->data.slider.min		 = 1;
	view->data.slider.max		 = 100;
	view->data.slider.button	 = gfx_view_make_button(parent);
	view->parent				 = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_slider(view_t *slider, cJSON *json) {
	view_t *button = slider->data.slider.button;
	if (button == NULL)
		return;

	if (button->inflate != NULL)
		button->inflate(button, cJSON_GetObjectItem(json, "button"));
	json_read_gfx_view_text(json, "text", &slider->data.slider.text);
	json_read_int(json, "value", &slider->data.slider.value);
	json_read_int(json, "min", &slider->data.slider.min);
	json_read_int(json, "max", &slider->data.slider.max);
}

static void gfx_view_free_slider(view_t *slider) {
	if (slider == NULL)
		return;
	gfx_view_free(slider->data.slider.button);
	free(slider->data.slider.text.text);
	free(slider);
}

static void gfx_view_measure_slider(view_t *slider) {
	view_t *button = slider->data.slider.button;
	if (button == NULL)
		return;

	memcpy(&button->rect, &slider->rect, sizeof(slider->rect));
	if (button->measure != NULL)
		button->measure(button);
	memcpy(&slider->rect, &button->rect, sizeof(slider->rect));
}

static void gfx_view_draw_slider(SDL_Renderer *renderer, view_t *slider) {
	view_t *button = slider->data.slider.button;
	if (button == NULL)
		return;

	int x	  = slider->rect.x;
	int y	  = slider->rect.y;
	int w	  = slider->rect.w;
	int h	  = slider->rect.h;
	int whalf = w / 2;
	int hhalf = h / 2;

	int max		 = slider->data.slider.max - slider->data.slider.min;
	int value	 = slider->data.slider.value - slider->data.slider.min;
	int slider_x = (value * (slider->rect.w - 8) / max) - 8;
	slider_x	 = max(slider_x, 0);

	memcpy(&button->rect, &slider->rect, sizeof(slider->rect));

	button->is_disabled = true;
	button->is_focused	= slider->is_focused;
	if (button->draw != NULL)
		button->draw(renderer, button);

	button->is_disabled = false;
	button->is_focused	= false;
	button->rect.x		= slider->rect.x + slider_x;
	button->rect.w		= 16;
	if (button->draw != NULL)
		button->draw(renderer, button);

	gfx_set_text_style(slider->data.slider.text.font, slider->data.slider.text.size, GFX_ALIGN_CENTER);
	// draw the text shadow
	gfx_set_color(renderer, button->data.button.fg_shadow);
	gfx_draw_text(renderer, x + whalf + 2, y + hhalf + 2, slider->data.slider.text.text);
	// draw the text
	gfx_set_color(
		renderer,
		slider->is_disabled	 ? button->data.button.fg_disabled
		: slider->is_focused ? button->data.button.fg_focused
							 : slider->data.slider.text.color
	);
	gfx_draw_text(renderer, x + whalf, y + hhalf, slider->data.slider.text.text);
}

static bool gfx_view_on_event_slider(view_t *slider, SDL_Event *e) {
	int min	  = slider->data.slider.min;
	int max	  = slider->data.slider.max;
	int value = slider->data.slider.value;

	int x = 0;
	switch (e->type) {
		case SDL_KEYDOWN:
			// change the value on Left/Right keypress
			if (e->key.keysym.sym == SDLK_RIGHT)
				value += 1;
			else if (e->key.keysym.sym == SDLK_LEFT)
				value -= 1;
			break;

		case SDL_MOUSEBUTTONDOWN:
			// capture all events if mouse grabs the button
			slider->in_event = true;
			// save the X position
			x = e->button.x - slider->rect.x;
			break;

		case SDL_MOUSEBUTTONUP:
			// disable the capture once it releases the button
			slider->in_event = false;
			return false;

		case SDL_MOUSEMOTION:
			if (!slider->in_event)
				return false;
			// save the X position
			x = e->motion.x - slider->rect.x;
			break;

		default:
			return false;
	}

	if (x != 0) {
		int range = max - min;
		value	  = round((double)(x * range) / (slider->rect.w - 8));
		value += min;
	}

	value = min(value, max);
	value = max(value, min);

	if (slider->data.slider.value != value) {
		slider->data.slider.value = value;
		if (slider->event.change != NULL)
			slider->event.change(slider, e);
	}

	return true;
}
