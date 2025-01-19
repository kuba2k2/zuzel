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

/**
 * Shift the player position history by 1.
 * Position 0 is not modified (will equal position 1).
 * Last position is removed from the list.
 *
 * The positions are not shifted if the player's head is the same
 * as their tail (player crashed/finished, and has already disappeared).
 *
 * @return whether positions were shifted
 */
bool player_position_shift(player_t *player) {
	if (player->pos[PLAYER_POS_NUM - 1].x == player->pos[0].x && player->pos[PLAYER_POS_NUM - 1].y == player->pos[0].y)
		return false;
	memmove(&player->pos[1], &player->pos[0], sizeof(*player->pos) * (PLAYER_POS_NUM - 1));
	return true;
}

/**
 * (Re)calculate player positions, starting at 'start'.
 * The starting position is not modified, but used to calculate
 * all following positions (higher time/lower index).
 * 'start' must point to player->pos[1] through player->pos[PLAYER_POS_NUM-1].
 */
void player_position_calculate(player_t *player, player_pos_t *start) {
	for (player_pos_t *next = start - 1; next >= player->pos; next--) {
		player_pos_t *prev = next + 1;

		next->time	= prev->time + 5;
		next->angle = prev->angle;
		next->speed = prev->speed;
		if (prev->direction == PLAYER_POS_LEFT) {
			next->angle += 2;
			if (next->angle > 359)
				next->angle -= 360;
			if (next->speed > 3.0)
				next->speed -= 0.048;
		} else {
			if (next->speed < 7.0)
				next->speed += 0.052;
		}

		double angle_rad = next->angle * M_PI / 180.0;
		next->x			 = prev->x + cos(angle_rad) * next->speed;
		next->y			 = prev->y - sin(angle_rad) * next->speed;
		next->direction	 = prev->direction;
		next->confirmed	 = player->is_local;
	}
}

/**
 * Check if the player advances to another lap, finishes the race
 * or crashes by passing the finish line incorrectly.
 * Update the player state if any of these happen.
 */
bool player_position_check_lap(player_t *player, player_pos_t *prev, player_pos_t *next) {
	// check if the player moves through half a lap
	if (!player->lap_can_advance && prev->y > 240.0 && next->y <= 240.0) {
		player->lap_can_advance = true;
		LT_I("Player: #%u not stuck anymore @ %u", player->id, next->time);
	}

	// check if the player finishes a lap
	if (prev->x <= 319.0 && next->x > 319.0) {
		if (player->lap == 4) {
			player->state = PLAYER_FINISHED;
			LT_I("Player: #%u finished the race @ %u", player->id, next->time);
			return true;
		}
		if (!player->lap_can_advance) {
			player->state = PLAYER_CRASHED;
			LT_I("Player: #%u tried to pass the finish line @ %u", player->id, next->time);
			return true;
		}
		player->lap++;
		LT_I("Player: #%u advanced to lap %d @ %u", player->id, player->lap, next->time);
		return true;
	}
	return false;
}

/**
 * Check if the position collides with a wall.
 * Update the player state if the player crashes.
 */
bool player_position_check_collision(player_t *player, player_pos_t *pos) {
	if (pos->y <= 92.0 || pos->y >= 388.0 ||
		(pos->x >= 150.0 && pos->x <= 489.0 && pos->y >= 188.0 && pos->y <= 292.0)) {
		goto crash;
	}
	double dy = pos->y - 240.0;
	if (pos->x <= 150.0) {
		double dx	= pos->x - 150.0;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist <= 52.0 || dist >= 148.0)
			goto crash;
	} else if (pos->x >= 489.0) {
		double dx	= pos->x - 489.0;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist <= 52.0 || dist >= 148.0)
			goto crash;
	}
	return false;
crash:
	player->state = PLAYER_CRASHED;
	LT_I("Player: #%u crashed into the wall @ %u", player->id, pos->time);
	return true;
}

bool player_loop(player_t *player) {
	if (!player_position_shift(player))
		// position unchanged - player is already gone
		return false;
	bool changed = false;

	player_pos_t *next = &player->pos[0];
	player_pos_t *prev = &player->pos[1];

	if (player->state == PLAYER_PLAYING) {
		player_position_calculate(player, prev);
		if (player_position_check_lap(player, prev, next))
			changed = true;
		else if (player_position_check_collision(player, next))
			changed = true;
		player->time = next->time;
	}

	return changed;
}
