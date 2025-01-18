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

static void player_add_position(player_t *player, double x, double y) {
	if (player->pos[99].x != player->pos[0].x || player->pos[99].y != player->pos[0].y) {
		for (int i = 99; i >= 1; i--) {
			player->pos[i].x = player->pos[i - 1].x;
			player->pos[i].y = player->pos[i - 1].y;
		}
		player->pos[0].x = x;
		player->pos[0].y = y;
	}
}

void player_loop(game_t *game, player_t *player) {
	double head_x = player->pos[0].x;
	double head_y = player->pos[0].y;

	const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);

	if (player->state == PLAYER_PLAYING) {
		if (keyboard_state[SDL_SCANCODE_RSHIFT]) {
			player->angle += 2;
			if (player->angle > 359)
				player->angle -= 360;
			if (player->speed > 3.0)
				player->speed -= 0.048;
		} else {
			if (player->speed < 7.0)
				player->speed += 0.052;
		}

		// calculate new head X and Y
		double angle_rad = (double)player->angle * M_PI / 180.0;
		head_x			 = head_x + cos(angle_rad) * player->speed;
		head_y			 = head_y - sin(angle_rad) * player->speed;
		// increment playing time
		player->time += 5;
		bool check_collision = true;

		// check if the player moves through half a lap
		if (!player->lap_can_advance && player->pos[0].y > 240.0 && head_y <= 240.0) {
			player->lap_can_advance = true;
			LT_I("Player: #%u not stuck anymore @ %u", player->id, player->time);
		}

		// check if the player finishes a lap
		if (player->pos[0].x <= 319.0 && head_x > 319.0) {
			if (player->lap == 4) {
				player->state	   = PLAYER_FINISHED;
				game->update_state = true;
				check_collision	   = false;
				LT_I("Player: #%u finished the race @ %u", player->id, player->time);
			} else if (!player->lap_can_advance) {
				player->state	   = PLAYER_CRASHED;
				game->update_state = true;
				check_collision	   = false;
				LT_I("Player: #%u tried to pass the finish line @ %u", player->id, player->time);
			} else {
				player->lap++;
				LT_I("Player: #%u advanced to lap %d @ %u", player->id, player->lap, player->time);
				// update game lap number
				if (player->lap > game->lap) {
					game->update_state = true;
					game->lap		   = player->lap;
				}
			}
		}

		// check if the player crashes into the wall (if still playing)
		if (check_collision) {
			bool colliding = false;
			if (head_y <= 92.0 || head_y >= 388.0 ||
				(head_x >= 150.0 && head_x <= 489.0 && head_y >= 188.0 && head_y <= 292.0)) {
				colliding = true;
			} else if (head_x <= 150.0) {
				double dx	= head_x - 150.0;
				double dy	= head_y - 240.0;
				double dist = sqrt(dx * dx + dy * dy);
				if (dist <= 52.0 || dist >= 148.0)
					colliding = true;
			} else if (head_x >= 489.0) {
				double dx2		 = head_x - 489.0;
				double dy		 = head_y - 240.0;
				double distance2 = sqrt(dx2 * dx2 + dy * dy);
				if (distance2 <= 52.0 || distance2 >= 148.0)
					colliding = true;
			}
			if (colliding) {
				player->state	   = PLAYER_CRASHED;
				game->update_state = true;
				LT_I("Player: #%u crashed into the wall @ %u", player->id, player->time);
			}
		}
	}

	player_add_position(player, head_x, head_y);
}
