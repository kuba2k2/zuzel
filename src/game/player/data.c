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
static const SDL_Scancode player_keys[] = {
	SDL_SCANCODE_RSHIFT,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_RCTRL,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_RALT,
	SDL_SCANCODE_LALT,
	SDL_SCANCODE_LEFT,
};

void player_set_color(game_t *game, player_t *player) {
	// first fetch a color randomly
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

void player_set_key(game_t *game, player_t *player) {
	// search the list to find a key that's not used
	SDL_WITH_MUTEX(game->mutex) {
		for (int i = 0; i < sizeof(player_keys) / sizeof(*player_keys); i++) {
			SDL_Scancode key = player_keys[i];
			player_t *out;
			DL_SEARCH_SCALAR(game->players, out, turn_key, key);
			if (out == NULL) {
				// player with this key not found, use it
				player->turn_key = key;
				break;
			}
		}
	}
}

const char *player_get_key_name(player_t *player) {
	switch (player->turn_key) {
		case SDL_SCANCODE_RSHIFT:
			return "Right Shift";
		case SDL_SCANCODE_LSHIFT:
			return "Left Shift";
		case SDL_SCANCODE_RCTRL:
			return "Right Ctrl";
		case SDL_SCANCODE_LCTRL:
			return "Left Ctrl";
		case SDL_SCANCODE_RALT:
			return "Right Alt";
		case SDL_SCANCODE_LALT:
			return "Left Alt";
		case SDL_SCANCODE_LEFT:
			return "Left Arrow";
		default:
			return "Unknown Key";
	}
}

void player_fill_data_pkt(game_t *game, player_t *player, pkt_player_data_t *pkt) {
	pkt->id	   = player->id;
	pkt->color = player->color;
	pkt->state = player->state;
	pkt->lap   = player->pos[0].lap;
	strcpy(pkt->name, player->name);
}

void player_reset_round(game_t *game) {
	BUILD_BUG_ON(PLAYER_POS_NUM < 20);
	player_t *player, *player_0 = NULL, *player_1 = NULL;
	int player_count = 0;
	DL_FOREACH(game->players, player) {
		if (player->state != PLAYER_READY)
			continue;
		// find the 1st and 2nd players that are ready
		if (player_0 == NULL)
			player_0 = player;
		else if (player_1 == NULL)
			player_1 = player;
		player_count++;
	}
	if (player_count == 0)
		return;

	// generate Y positions
	player_0 = game->players;
	// generate a pseudo-random starting position that's the same for every connected client
	uint32_t seed = (player_0->id * game->round * game->speed * 10) * 1103515245 + 12345;
	seed		  = (uint32_t)(seed / 65536) % 32768;
	// apply the position
	player_0->pos[0].y = 310.0 + (seed % 4) * 20.0;
	LT_I("Player #%u starting Y position: %f", player_0->id, player_0->pos[0].y);

	if (player_count > 2) {
		int player_idx = 0;
		DL_FOREACH(game->players, player) {
			if (player->state != PLAYER_READY)
				continue;
			if (player_idx == 0) {
				player_idx++;
				continue;
			}
			player->pos[0].y = player_0->pos[0].y + (double)player_idx * 20.0;
			if (player_idx >= 4)
				player->pos[0].y += 10.0;
			while (player->pos[0].y > 370.0)
				player->pos[0].y -= 80.0;
			player_idx++;
		}
	} else if (player_count == 2 && player_1 != NULL) {
		if (player_0->pos[0].y >= 350.0)
			player_1->pos[0].y = player_0->pos[0].y - 40.0;
		else
			player_1->pos[0].y = player_0->pos[0].y + 40.0;
	}

	DL_FOREACH(game->players, player) {
		if (player->state != PLAYER_READY)
			continue;
		// reset player data
		player->lap_can_advance = false;
		player->is_in_round		= true;
		player->round_points	= 0;
		// reset all player positions
		player->pos[0].time		 = 0;
		player->pos[0].angle	 = 0;
		player->pos[0].speed	 = 1.0;
		player->pos[0].x		 = 320.0;
		player->pos[0].lap		 = 1;
		player->pos[0].direction = PLAYER_POS_FORWARD;
		player->pos[0].confirmed = true;
		for (int i = 1; i < 20; i++) {
			player->pos[i]	 = player->pos[0];
			player->pos[i].x = 320 - (i + 1);
		}
		for (int i = 20; i < PLAYER_POS_NUM; i++) {
			player->pos[i]	 = player->pos[0];
			player->pos[i].x = 300;
		}
		// reset all future keypress events
		player_keypress_t *keypress, *tmp;
		DL_FOREACH_SAFE(player->keypress, keypress, tmp) {
			DL_DELETE(player->keypress, keypress);
			free(keypress);
		}
	}
}
