// Copyright (c) Kuba Szczodrzyński 2024-12-19.

#include "view.h"

static void gfx_view_inflate_input(view_t *input, cJSON *json);
static void gfx_view_measure_input(view_t *input);
static void gfx_view_draw_input(SDL_Renderer *renderer, view_t *input);
static bool gfx_view_on_event_input(view_t *input, SDL_Event *e);

view_t *gfx_view_make_input(view_t *parent) {
	view_t *view;
	MALLOC(view, sizeof(*view), return NULL);

	view->type						   = VIEW_TYPE_BUTTON;
	view->inflate					   = gfx_view_inflate_input;
	view->measure					   = gfx_view_measure_input;
	view->draw						   = gfx_view_draw_input;
	view->on_event					   = gfx_view_on_event_input;
	view->is_focusable				   = true;
	view->data.input.text.color		   = 0xE0E0E0;
	view->data.input.text.size		   = FONT_SIZE_DEFAULT;
	view->data.input.placeholder.color = 0x505050;
	view->data.input.placeholder.size  = FONT_SIZE_DEFAULT;
	view->data.input.max_length		   = 32;
	view->parent					   = parent;

	view->id = gfx_view_make_id(view);
	return view;
}

static void gfx_view_inflate_input(view_t *input, cJSON *json) {
	json_read_gfx_view_text(json, "text", &input->data.input.text);
	json_read_gfx_view_text(json, "placeholder", &input->data.input.placeholder);
	json_read_int(json, "max_length", &input->data.input.max_length);

	MALLOC(input->data.input.value, input->data.input.max_length + 2, return);
	if (input->data.input.text.text != NULL) {
		strncpy(input->data.input.value, input->data.input.text.text, input->data.input.max_length);
		input->data.input.pos = strlen(input->data.input.value);
	}
}

static void gfx_view_measure_input(view_t *input) {
	if (input->rect.w == 0)
		input->rect.w = 400;
	if (input->rect.h == 0)
		input->rect.h = 40;
}

static void gfx_view_draw_input(SDL_Renderer *renderer, view_t *input) {
	int x = input->rect.x;
	int y = input->rect.y;
	int w = input->rect.w;
	int h = input->rect.h;

	// draw the input outline
	gfx_set_color(renderer, 0x000000);
	gfx_draw_rect(renderer, x, y, w, h, true);
	gfx_set_color(renderer, 0xA0A0A0);
	gfx_draw_rect(renderer, x + 2, y + 2, w - 4, h - 4, false);
	gfx_draw_rect(renderer, x + 3, y + 3, w - 6, h - 6, false);

	char *text	 = input->data.input.value;
	int text_len = strlen(text);
	int pos		 = input->data.input.pos;

	bool focused = input->is_focused && !input->in_event;

	unsigned int text_color		   = input->is_disabled ? 0xA0A0A0 : focused ? 0xFFFFA0 : input->data.input.text.color;
	unsigned int placeholder_color = focused ? 0xA0A050 : input->data.input.placeholder.color;

	gfx_set_text_style(
		input->data.input.text.font,
		input->data.input.text.size,
		GFX_ALIGN_LEFT | GFX_ALIGN_CENTER_VERTICAL
	);

	int xc = x + 10;
	int yc = y + h / 2;

	if (text[0] == '\0') {
		gfx_set_color(renderer, placeholder_color);
		gfx_draw_text(renderer, x + 10, y + h / 2, input->data.input.placeholder.text);
	} else {
		char ch = '\0';
		if (pos < text_len) {
			ch		  = text[pos];
			text[pos] = '\0';
		}
		gfx_set_color(renderer, 0x383838);
		gfx_draw_text(renderer, xc + 2, yc + 2, text);
		gfx_set_color(renderer, text_color);
		xc = gfx_draw_text(renderer, xc, yc, text);
		if (pos < text_len) {
			text[pos] = ch;
		}
	}

	if (input->in_event && !input->is_disabled) {
		gfx_set_color(renderer, input->data.input.text.color);
		gfx_draw_rect(renderer, xc + 2, yc - 10, 2, 20, false);
		xc += 6;
	}

	if (pos < text_len) {
		text += pos;
		gfx_set_color(renderer, 0x383838);
		gfx_draw_text(renderer, xc + 2, yc + 2, text);
		gfx_set_color(renderer, text_color);
		gfx_draw_text(renderer, xc, yc, text);
	}
}

static bool gfx_view_on_event_input(view_t *input, SDL_Event *e) {
	if (e->type == SDL_MOUSEBUTTONDOWN) {
		if (GFX_VIEW_IN_BOX(input, e->button.x, e->button.y)) {
			// start capturing events on click inside the input
			input->in_event		  = true;
			input->data.input.pos = strlen(input->data.input.value);
			SDL_SetTextInputRect(&input->rect);
			SDL_StartTextInput();
		} else {
			// disable capturing if clicked outside
			input->in_event = false;
			SDL_StopTextInput();
		}
		return input->in_event;
	}
	if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_RETURN) {
		// start/stop capturing events on Enter keypress
		if (!input->in_event) {
			input->in_event		  = true;
			input->data.input.pos = strlen(input->data.input.value);
			SDL_SetTextInputRect(&input->rect);
			SDL_StartTextInput();
		} else {
			input->in_event = false;
			SDL_StopTextInput();
		}
		return true;
	}

	if (!input->in_event)
		return false;

	char *text	 = input->data.input.value;
	int text_len = strlen(text);
	int max_len	 = input->data.input.max_length;
	int pos		 = input->data.input.pos;
	bool changed = false;
	char *chars	 = NULL;

	switch (e->type) {
		case SDL_KEYDOWN:
			// move the cursor on Left/Right keypress
			if (e->key.keysym.sym == SDLK_RIGHT)
				pos += 1;
			else if (e->key.keysym.sym == SDLK_LEFT)
				pos -= 1;
			// erase characters on Backspace/Delete
			else if (e->key.keysym.sym == SDLK_BACKSPACE && pos > 0) {
				memmove(&text[pos - 1], &text[pos], text_len - pos + 1);
				text_len -= 1;
				pos -= 1;
				changed = true;
			} else if (e->key.keysym.sym == SDLK_DELETE && pos < text_len) {
				memmove(&text[pos], &text[pos + 1], text_len - pos + 1);
				text_len -= 1;
				changed = true;
			}
			break;

		case SDL_TEXTINPUT:
			// handle text input events
			chars = e->text.text;
			break;
	}

	if (chars != NULL && text_len < max_len) {
		int chars_len = strlen(chars);
		chars_len	  = min(chars_len, max_len - text_len);
		memmove(&text[pos + chars_len], &text[pos], text_len - pos + 1);
		memcpy(&text[pos], chars, chars_len);
		text_len += chars_len;
		pos += chars_len;
		changed = true;
		// replace non-ASCII chars with '?'
		char ch;
		while ((ch = *text) != '\0') {
			if (ch < ' ' || ch > '~')
				*text = '?';
			text++;
		}
	}

	if (input->data.input.pos != pos) {
		pos = min(pos, text_len);
		pos = max(pos, 0);
		// assign the new, valid, position
		input->data.input.pos = pos;
	}

	if (changed && input->event.change != NULL)
		input->event.change(input, e);

	return true;
}
