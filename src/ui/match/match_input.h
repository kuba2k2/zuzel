// Copyright (c) Kuba Szczodrzyński 2025-1-30.

#pragma once

#include "include.h"

typedef void (*match_input_on_ready_t)(ui_t *);
typedef void (*match_input_on_quit_t)(ui_t *);

bool match_input_process_key_event(
	ui_t *ui,
	SDL_Scancode key,
	bool pressed,
	match_input_on_ready_t on_ready,
	match_input_on_quit_t on_quit
);
bool match_input_process_mouse_event(
	ui_t *ui,
	int x,
	int y,
	bool pressed,
	match_input_on_ready_t on_ready,
	match_input_on_quit_t on_quit
);
