// Copyright (c) Kuba Szczodrzyński 2024-12-18.

#include "view.h"

static void gfx_view_inflate_button(view_t *button, cJSON *json);
static void gfx_view_measure_button(view_t *button);
static void gfx_view_draw_button(SDL_Renderer *renderer, view_t *button);

view_t *gfx_view_make_button(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type					 = VIEW_TYPE_BUTTON;
	view->inflate				 = gfx_view_inflate_button;
	view->measure				 = gfx_view_measure_button;
	view->draw					 = gfx_view_draw_button;
	view->data.button.text.color = GFX_COLOR_BRIGHT_WHITE;
	view->data.button.text.size	 = FONT_SIZE_DEFAULT;
	view->parent				 = parent;

	return view;
}

static void gfx_view_inflate_button(view_t *button, cJSON *json) {
	json_read_gfx_view_text(json, "text", &button->data.button.text);
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

	int x = button->rect.x;
	int y = button->rect.y;
	int w = button->rect.w;
	int h = button->rect.h;

	// draw the button outline
	gfx_set_color(renderer, 0x000000);
	gfx_draw_rect(renderer, x, y, w, h, true);
	x += 2;
	y += 2;
	w -= 4;
	h -= 4;

	// draw the button body (main background color)
	gfx_set_color(renderer, button->is_disabled ? 0x2A2A2A : button->is_focused ? 0x7E88BF : 0x707070);
	gfx_draw_rect(renderer, x, y, w, h, true);

	// draw the face texture
	int whalf = w / 2;
	int hhalf = h / 2;
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

	// add bezels
	gfx_set_color(renderer, 0x55FFFFFF);
	gfx_draw_rect(renderer, x, y, w - 4, 2, false);
	gfx_draw_rect(renderer, x, y, 2, h - 4, false);
	gfx_set_color(renderer, 0x55000000);
	gfx_draw_rect(renderer, x + w - 4, y, 4, h, true);
	gfx_draw_rect(renderer, x, y + h - 4, w - 4, 4, true);

	// finally, draw the text
	gfx_set_color(renderer, button->data.button.text.color);
	gfx_set_text_style(button->data.button.text.font, button->data.button.text.size, GFX_ALIGN_CENTER);
	gfx_draw_text(renderer, x + w / 2, y + h / 2, button->data.button.text.text);
}
