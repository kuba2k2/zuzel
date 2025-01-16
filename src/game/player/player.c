// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#include "player.h"

static const unsigned int player_colors[] = {
	GFX_COLOR_BRIGHT_RED,	  // 0xFF5555
	GFX_COLOR_WHITE,		  // 0xAAAAAA
	GFX_COLOR_BRIGHT_BLUE,	  // 0x5555FF
	GFX_COLOR_BRIGHT_YELLOW,  // 0xFFFF55
	GFX_COLOR_BRIGHT_GREEN,	  // 0x55FF55
	GFX_COLOR_BRIGHT_CYAN,	  // 0x55FFFF
	GFX_COLOR_BRIGHT_MAGENTA, // 0xFF55FF
	0xFF8200,
	0xFF00BE,
	0x41FF00,
};

player_t *player_init(game_t *game, char *name) {
	player_t *player;
	MALLOC(player, sizeof(*player), goto cleanup);

	SDL_WITH_MUTEX(player->mutex) {
		player->game  = game;
		player->state = PLAYER_IDLE;

		do {
			player->id = rand() % (100 - 10) + 10; // 10..99
		} while (game_get_player_by_id(game, player->id) != NULL);

		if (name != NULL)
			strncpy2(player->name, name, sizeof(player->name) - 1);

		// set player color - first fetch a color randomly
		player->color = player_colors[rand() % (sizeof(player_colors) / sizeof(*player_colors))];
		// then search the list to find a color that's not used
		SDL_WITH_MUTEX(game->mutex) {
			for (int i = 0; i < sizeof(player_colors) / sizeof(*player_colors); i++) {
				unsigned int color = player_colors[i];
				player_t *out;
				DL_SEARCH_SCALAR(game->players, out, color, color);
				if (out == NULL) {
					// player with this color not found, use it
					player->color = color;
					break;
				}
			}
		}
	}

	return player;

cleanup:
	player_free(player);
	return NULL;
}

void player_free(player_t *player) {
	if (player == NULL)
		return;
	SDL_DestroyMutex(player->mutex);
	SDL_DestroyTexture(player->texture);
	free(player);
}
