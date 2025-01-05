// Copyright (c) Kuba Szczodrzyński 2024-12-21.

#include "ui.h"

#include "font_res.h"
#include "ui_res.h"

ui_t *ui_init(SDL_Renderer *renderer) {
	ui_t *ui;
	MALLOC(ui, sizeof(*ui), return NULL);

	// initialize SDL for the UI
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

	// load all used fonts
	gfx_load_font(0, NULL, FONT_BIOS_BFN);
	gfx_load_font(1, NULL, FONT_SIMP_CHR);
	gfx_load_font(2, NULL, FONT_TSCR_CHR);
	gfx_load_font(3, NULL, FONT_SANS_CHR);

	// inflate all UI fragments (menus etc.)
	fragment_init_all(ui->fragments, ui);

	return ui;

free_menu:
	ui_free(ui);
	return NULL;
}

int ui_run(ui_t *ui) {
	fragment_t *prev_fragment = NULL;
	while (1) {
		fragment_t *fragment = ui->fragments[ui->state];

		SDL_SetRenderTarget(ui->renderer, NULL);
		gfx_set_color(ui->renderer, 0x000000);
		SDL_RenderClear(ui->renderer);

		if (fragment != NULL) {
			if (fragment != prev_fragment) {
				// call on_hide/on_show
				if (prev_fragment != NULL && prev_fragment->on_hide != NULL)
					prev_fragment->on_hide(ui, prev_fragment, NULL);
				if (fragment->on_show != NULL)
					fragment->on_show(ui, fragment, NULL);
			}
			if (fragment != prev_fragment || ui->force_layout) {
				gfx_view_measure(fragment->views);
				gfx_view_layout(fragment->views);
			}
			prev_fragment	 = fragment;
			ui->force_layout = false;
			gfx_view_draw(ui->renderer, fragment->views);
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
				else if (e.key.keysym.sym == SDLK_F5)
					prev_fragment = NULL, fragment_reload(fragment, ui);
				else if (e.key.keysym.sym == SDLK_F10)
					net_client_stop(), net_server_stop(), ui_state_prev(ui);
				break;

			default:
				if (fragment == NULL)
					break;
				if (fragment->on_event != NULL && fragment->on_event(ui, fragment, &e) == true)
					break;
				if (fragment->views != NULL && gfx_view_on_event(fragment->views, &e) == true)
					break;
				break;
		}

		// free custom packets that need it
		if (e.type == SDL_USEREVENT_PACKET)
			free(e.user.data1);
	}
}

void ui_free(ui_t *ui) {
	if (ui == NULL)
		return;
	free(ui->connection.address);
	free(ui->connection.key);
	net_client_stop();
	net_server_stop();
	for (ui_state_t state = UI_STATE_MAIN; state < UI_STATE_MAX; state++) {
		fragment_t *fragment = ui->fragments[state];
		if (fragment != NULL)
			gfx_view_free(fragment->views);
	}
	SDL_DestroyTexture(ui->texture);
	free(ui);
}
