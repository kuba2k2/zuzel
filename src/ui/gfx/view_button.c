// Copyright (c) Kuba Szczodrzyński 2024-12-18.

#include "view.h"

static void gfx_view_inflate_button(view_t *button, cJSON *json, const view_inflate_on_event_t *on_event);
static void gfx_view_clone_button(view_t *src, view_t *dst);
static void gfx_view_free_button(view_t *button);
static void gfx_view_measure_button(view_t *button);
static void gfx_view_draw_button(SDL_Renderer *renderer, view_t *button);
static bool gfx_view_on_event_button(view_t *button, SDL_Event *e, void *param);

view_t *gfx_view_make_button(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type					  = VIEW_TYPE_BUTTON;
	view->inflate				  = gfx_view_inflate_button;
	view->clone					  = gfx_view_clone_button;
	view->free					  = gfx_view_free_button;
	view->measure				  = gfx_view_measure_button;
	view->draw					  = gfx_view_draw_button;
	view->on_event				  = gfx_view_on_event_button;
	view->is_focusable			  = true;
	view->data.button.text.color  = 0xE0E0E0;
	view->data.button.text.size	  = 7;
	view->data.button.bg_color	  = 0x707070;
	view->data.button.bg_focused  = 0x7E88BF;
	view->data.button.bg_disabled = 0x2A2A2A;
	view->data.button.fg_shadow	  = 0x383838;
	view->data.button.fg_disabled = 0xA0A0A0;
	view->data.button.fg_focused  = 0xFFFFA0;
	view->parent				  = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_button(view_t *button, cJSON *json, const view_inflate_on_event_t *on_event) {
	json_read_gfx_view_text(json, "text", &button->data.button.text);
	json_read_gfx_color(json, "bg_color", &button->data.button.bg_color);
	json_read_gfx_color(json, "bg_focus", &button->data.button.bg_focused);
	json_read_gfx_color(json, "bg_disabled", &button->data.button.bg_disabled);
	json_read_gfx_color(json, "fg_shadow", &button->data.button.fg_shadow);
	json_read_gfx_color(json, "fg_focus", &button->data.button.fg_focused);
	json_read_gfx_color(json, "fg_disabled", &button->data.button.fg_disabled);
	json_read_bool(json, "is_flat", &button->data.button.is_flat);
}

static void gfx_view_clone_button(view_t *src, view_t *dst) {
	if (src->data.button.text.text != NULL)
		dst->data.button.text.text = strdup(src->data.button.text.text);
}

static void gfx_view_free_button(view_t *button) {
	if (button == NULL)
		return;
	free(button->data.button.text.text);
	free(button);
}

static void gfx_view_measure_button(view_t *button) {
	gfx_set_text_style(button->data.button.text.font, button->data.button.text.size, GFX_ALIGN_CENTER);
	if (button->rect.w == 0) {
		int width = gfx_get_text_width(button->data.button.text.text, false);
		width += 40 * 2;
		button->rect.w = max(width, 192) + 4;
	}
	if (button->rect.h == 0) {
		int height = gfx_get_text_height(button->data.button.text.text);
		height += 10 * 2;
		button->rect.h = max(height, 36) + 4;
	}
}

static void gfx_view_draw_button(SDL_Renderer *renderer, view_t *button) {
	// load the button face texture
	if (texture_button_face == NULL) {
		texture_load(
			renderer,
			&texture_button_face,
			texture_button_face_width,
			texture_button_face_height,
			texture_button_face_data
		);
	}

	int x		 = button->rect.x;
	int y		 = button->rect.y;
	int w		 = button->rect.w;
	int h		 = button->rect.h;
	bool is_flat = button->data.button.is_flat;

	// draw the button outline
	if (!is_flat) {
		gfx_set_color(renderer, 0x000000);
		gfx_draw_rect(renderer, x, y, w, h, true);
	}
	x += 2;
	y += 2;
	w -= 4;
	h -= 4;

	int whalf = w / 2;
	int hhalf = h / 2;

	if (!is_flat) {
		// draw the button body (main background color)
		gfx_set_color(
			renderer,
			button->is_disabled	 ? button->data.button.bg_disabled
			: button->is_focused ? button->data.button.bg_focused
								 : button->data.button.bg_color
		);
		gfx_draw_rect(renderer, x, y, w, h, true);

		// draw the face texture
		for (int tex_x = 0; tex_x < whalf; tex_x += texture_button_face_width) {
			for (int tex_y = 0; tex_y < hhalf; tex_y += texture_button_face_height) {
				SDL_Rect src = {
					.x = 0,
					.y = 0,
					.w = min(whalf, texture_button_face_width),
					.h = min(hhalf, texture_button_face_height),
				};
				SDL_Rect dst = {
					.x = x + tex_x * 2,
					.y = y + tex_y * 2,
					.w = min(whalf - tex_x, texture_button_face_width) * 2,
					.h = min(hhalf - tex_y, texture_button_face_height) * 2,
				};
				SDL_RenderCopy(renderer, texture_button_face, &src, &dst);
			}
		}
	} else if (button->is_focused) {
		unsigned int bg_focused = button->data.button.bg_focused;
		bg_focused &= 0xFFFFFF;
		bg_focused |= 0x40000000;
		gfx_set_color(renderer, bg_focused);
		gfx_draw_rect(renderer, x, y, w, h, true);
	}

	gfx_set_text_style(button->data.button.text.font, button->data.button.text.size, GFX_ALIGN_CENTER);

	if (!button->is_disabled && !is_flat) {
		// add bezels if not disabled
		gfx_set_color(renderer, 0x55FFFFFF);
		gfx_draw_rect(renderer, x, y, w, 2, false);
		gfx_draw_rect(renderer, x, y + 2, 2, h - 2, false);
		gfx_set_color(renderer, 0x55000000);
		gfx_draw_rect(renderer, x + w - 2, y, 2, h, true);
		gfx_draw_rect(renderer, x, y + h - 4, w - 2, 4, true);

		// draw the text shadow
		gfx_set_color(renderer, button->data.button.fg_shadow);
		gfx_draw_text(renderer, x + whalf + 2, y + hhalf + 2, button->data.button.text.text);
	}

	// finally, draw the text
	gfx_set_color(
		renderer,
		button->is_disabled	 ? button->data.button.fg_disabled
		: button->is_focused ? button->data.button.fg_focused
							 : button->data.button.text.color
	);
	gfx_draw_text(renderer, x + whalf, y + hhalf, button->data.button.text.text);
}

static bool gfx_view_on_event_button(view_t *button, SDL_Event *e, void *param) {
	switch (e->type) {
		case SDL_KEYDOWN:
			// send a 'press' event on Enter keypress
			if (e->key.keysym.sym == SDLK_RETURN && button->event.press && !button->is_disabled)
				return button->event.press(button, e, param);
			break;

		case SDL_MOUSEBUTTONDOWN:
			// capture all events if mouse grabs the button
			button->in_event = true;
			break;

		case SDL_MOUSEBUTTONUP:
			// only send a 'press' event if the button is still focused
			if (button->in_event && button->is_focused && button->event.press && !button->is_disabled) {
				button->in_event = false;
				return button->event.press(button, e, param);
			}
			// disable the capture
			button->in_event = false;
			break;
	}
	return false;
}
