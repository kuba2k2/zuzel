// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

#include "ui_res.h"

// menu_main.c
extern const view_inflate_on_event_t menu_main_on_event[];
// menu_server_new.c
extern const view_inflate_on_event_t menu_server_new_on_event[];
// menu_server_join.c
extern const view_inflate_on_event_t menu_server_join_on_event[];

// views to inflate
const unsigned char *ui_states_json[UI_STATE_MAX] = {
	[UI_STATE_MENU_MAIN]		= UI_MENU_MAIN_JSON,
	[UI_STATE_MENU_SERVER_NEW]	= UI_MENU_SERVER_NEW_JSON,
	[UI_STATE_MENU_SERVER_JOIN] = UI_MENU_SERVER_JOIN_JSON,
};
const view_inflate_on_event_t *ui_states_on_event[UI_STATE_MAX] = {
	[UI_STATE_MENU_MAIN]		= menu_main_on_event,
	[UI_STATE_MENU_SERVER_NEW]	= menu_server_new_on_event,
	[UI_STATE_MENU_SERVER_JOIN] = menu_server_join_on_event,
};

ui_t *ui_init(SDL_Renderer *renderer) {
	ui_t *ui;
	MALLOC(ui, sizeof(*ui), return NULL);

	ui->renderer = renderer;
	ui->texture	 = SDL_CreateTexture(
		 ui->renderer,
		 SDL_PIXELFORMAT_RGBA8888,
		 SDL_TEXTUREACCESS_TARGET,
		 SETTINGS->screen.width,
		 SETTINGS->screen.height
	 );
	if (ui->texture == NULL)
		SDL_ERROR("SDL_CreateTexture()", goto free_menu);
	SDL_SetTextureBlendMode(ui->texture, SDL_BLENDMODE_BLEND);

	// inflate views for all UI states
	for (ui_state_t state = UI_STATE_MENU_MAIN; state < UI_STATE_MAX; state++) {
		if (ui_states_json[state] == NULL)
			continue;
		cJSON *json = cJSON_Parse((const char *)ui_states_json[state]);
		if (json == NULL)
			LT_E("JSON parse failed: %s", cJSON_GetErrorPtr());
		ui->views[state] = gfx_view_inflate(json, NULL, ui_states_on_event[state]);
		cJSON_Delete(json);
		if (ui->views[state] == NULL)
			LT_ERR(E, goto free_menu, "View inflate failed for state %d", state);
		gfx_view_set_event_param(ui->views[state], NULL, ui);
	}

	return ui;

free_menu:
	ui_free(ui);
	return NULL;
}

int ui_run(ui_t *ui) {
	view_t *prev_views = NULL;
	while (1) {
		view_t *views = ui->views[ui->state];

		SDL_SetRenderTarget(ui->renderer, NULL);
		gfx_set_color(ui->renderer, 0x000000);
		SDL_RenderClear(ui->renderer);

		if (views != NULL) {
			if (views != prev_views) {
				gfx_view_measure(views);
				gfx_view_layout(views);
				prev_views = views;
			}
			gfx_view_draw(ui->renderer, views);
		}
		SDL_RenderPresent(ui->renderer);

		SDL_Event e;
		SDL_WaitEvent(&e);
		switch (e.type) {
			case SDL_QUIT:
				return 0;

			case SDL_KEYUP:
				if (e.key.keysym.sym == SDLK_F1)
					gfx_view_bounding_box = !gfx_view_bounding_box;
				continue;

			case SDL_USEREVENT:
				break;

			default:
				if (views != NULL)
					gfx_view_on_event(views, &e);
				continue;
		}

		// SDL_USEREVENT
		switch (e.user.code) {
			case UI_EVENT_ERROR:
				break;
		}
	}
}

void ui_free(ui_t *ui) {
	if (ui == NULL)
		return;
	for (ui_state_t state = UI_STATE_MENU_MAIN; state < UI_STATE_MAX; state++) {
		gfx_view_free(ui->views[state]);
	}
	SDL_DestroyTexture(ui->texture);
	free(ui);
}
