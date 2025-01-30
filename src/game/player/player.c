// Copyright (c) Kuba Szczodrzyński 2025-1-15.

#include "player.h"

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

		player_set_color(game, player);
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
	if (player->pos[PLAYER_POS_NUM - 1].x == player->pos[0].x &&
		player->pos[PLAYER_POS_NUM - 1].y == player->pos[0].y) {
		player->is_in_round = false;
		return false;
	}
	memmove(&player->pos[1], &player->pos[0], sizeof(*player->pos) * (PLAYER_POS_NUM - 1));
	return true;
}

/**
 * (Re)calculate player positions, starting at 'start'.
 * The starting position is not modified, but used to calculate
 * all following positions (higher time/lower index).
 * 'start' must point to player->pos[1] through player->pos[PLAYER_POS_NUM-1].
 * The player's state will be updated based on lap advancement and collision.
 */
bool player_position_calculate(player_t *player, player_pos_t *start) {
	bool changed = false;
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
		next->lap		 = prev->lap;
		next->direction	 = prev->direction;
		next->confirmed	 = player->is_local;

		if (player_position_check_lap(player, prev, next))
			changed = true;
		else if (player_position_check_collision(player, next))
			changed = true;
	}
	return changed;
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
		if (prev->lap == 4) {
			player->state = PLAYER_FINISHED;
			LT_I("Player: #%u finished the race @ %u", player->id, next->time);
			return true;
		}
		if (!player->lap_can_advance) {
			player->state = PLAYER_CRASHED;
			LT_I("Player: #%u tried to pass the finish line @ %u", player->id, next->time);
			return true;
		}
		next->lap = prev->lap + 1;
		LT_I("Player: #%u advanced to lap %d @ %u", player->id, next->lap, next->time);
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

/**
 * Process a keypress event.
 * Apply the new direction to the player's position.
 * If 'time' points to any of the previous positions, recalculate.
 * Mark all positions older than 'time' as confirmed.
 *
 * @return whether an unconfirmed position was found by the specified time
 */
bool player_position_process_direction(player_t *player, unsigned int time, player_pos_dir_t direction) {
	player_pos_t *player_pos = NULL;

	if (player->pos[0].time == time) {
		// keypress time is in the latest player position, no need for recalculation
		player_pos			  = &player->pos[0];
		player_pos->direction = direction;
	} else {
		// keypress time points to an older position, find it and recalculate all following positions
		int max_time = 0;
		for (int i = 1; i < PLAYER_POS_NUM; i++) {
			max_time = max(max_time, player->pos[i].time);
			if (player->pos[i].time != time)
				continue;
			player_pos = &player->pos[i];
			break;
		}
		if (player_pos == NULL || player_pos->confirmed)
			return false;
		// set the player's direction starting in the position at 'time'
		player_pos->direction = direction;
		// consider the player still alive
		player->state = PLAYER_PLAYING;
		// recalculate all positions following this one
		// will also reassign player state
		player_position_calculate(player, player_pos);
	}

	player_pos->confirmed = true;
	// mark all older positions as confirmed
	while (++player_pos < player->pos + PLAYER_POS_NUM) {
		if (player_pos->confirmed)
			break;
		player_pos->confirmed = true;
	}
	return true;
}

/**
 * Process a keypress event from a remote player.
 * If 'time' points to a yet-non-existent position, save the keypress
 * event for future recalculation.
 *
 * @return whether an unconfirmed position was found by the specified time
 */
bool player_position_remote_keypress(player_t *player, unsigned int time, player_pos_dir_t direction) {
	if (player->is_local)
		// ignore key events for local players
		return false;

	if (player_position_process_direction(player, time, direction))
		// keypress event processed, nothing else to do
		return true;

	// position not found, the client's loop must be running too quickly
	player_keypress_t *keypress;
	MALLOC(keypress, sizeof(*keypress), return false);
	// save the 'future' keypress in the player's structure
	keypress->time		= time;
	keypress->direction = direction;
	DL_APPEND(player->keypress, keypress);

	return false;
}

/**
 * Process any future keypress events saved for this player.
 *
 * @return whether any future keypress events were used to update the player's position(s)
 */
bool player_position_future_keypress(player_t *player) {
	if (player->is_local)
		// ignore key events for local players
		return false;

	bool processed = false;
	player_keypress_t *keypress, *tmp;
	DL_FOREACH_SAFE(player->keypress, keypress, tmp) {
		if (player_position_process_direction(player, keypress->time, keypress->direction) == false)
			// keypress not processed
			continue;
		// keypress processed, remove from the list
		DL_DELETE(player->keypress, keypress);
		free(keypress);
		processed = true;
	}
	return processed;
}

bool player_loop(player_t *player) {
	// process any future keypress events *before* shifting the position history
	player_position_future_keypress(player);

	if (!player_position_shift(player))
		// position unchanged - player is already gone
		return false;

	// calculate positions for players that are still alive
	bool changed = false;
	if (player->state == PLAYER_PLAYING) {
		changed		 = player_position_calculate(player, &player->pos[1]);
		player->time = player->pos[0].time;
	}

	return changed;
}
