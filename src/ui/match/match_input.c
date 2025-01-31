// Copyright (c) Kuba Szczodrzyński 2025-1-30.

#include "match_input.h"

#define GAME (ui->game)

static player_t *match_input_get_player_by_key(game_t *game, SDL_Scancode key, bool *key_ready, bool *key_quit) {
	player_t *player  = NULL;
	bool all_finished = true;

	DL_FOREACH(game->players, player) {
		if (!player->is_local)
			continue;
		if (player->state == PLAYER_PLAYING || player->state == PLAYER_READY)
			// some players are still playing or are already ready
			all_finished = false;
		if (player->state == PLAYER_PLAYING && player->turn_key == key)
			// player is still alive and the turn_key matches
			break;
	}

	if (all_finished) {
		if (key == SDL_SCANCODE_RETURN)
			*key_ready = true;
		else if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_BACKSPACE)
			*key_quit = true;
		return NULL;
	}

	return player;
}

/**
 * Process key events for local players. Will directly update players' data structures and states.
 * When updated, packets will be sent to the server.
 *
 * @param ui ui_t*
 * @param key SDL key scancode
 * @param pressed whether key is pressed
 * @param on_ready callback invoked when players are ready
 * @param on_quit callback invoked when players want to quit
 * @return true
 */
bool match_input_process_key_event(
	ui_t *ui,
	SDL_Scancode key,
	bool pressed,
	match_input_on_ready_t on_ready,
	match_input_on_quit_t on_quit
) {
	bool key_ready = false, key_quit = false;
	player_t *player = match_input_get_player_by_key(GAME, key, &key_ready, &key_quit);

	SDL_LOCK_MUTEX(GAME->mutex);

	if (key_ready) {
		// make all local players ready
		DL_FOREACH(GAME->players, player) {
			if (!player->is_local)
				continue;
			player->state = PLAYER_READY;
			game_request_send_update(GAME, false, player->id);
		}
		if (on_ready != NULL)
			on_ready(ui);
	} else if (key_quit) {
		// quit the game
		if (on_quit != NULL)
			on_quit(ui);
	} else if (player != NULL) {
		// send a keypress event for a single player
		player_pos_dir_t direction = pressed ? PLAYER_POS_LEFT : PLAYER_POS_FORWARD;
		SDL_LOCK_MUTEX(player->mutex);
		if (player->pos[0].direction != direction) {
			// direction changed, assign to the local player
			player->pos[0].direction = direction;
			player->pos[0].confirmed = true;
			// send player keypress packet to server
			pkt_player_keypress_t pkt = {
				.hdr.type  = PKT_PLAYER_KEYPRESS,
				.id		   = player->id,
				.time	   = player->pos[0].time,
				.direction = direction,
			};
			net_pkt_send_pipe(GAME->endpoints, (pkt_t *)&pkt);
		}
		SDL_UNLOCK_MUTEX(player->mutex);
	}

	SDL_UNLOCK_MUTEX(GAME->mutex);

	return true;
}

bool match_input_process_mouse_event(
	ui_t *ui,
	int x,
	int y,
	bool pressed,
	match_input_on_ready_t on_ready,
	match_input_on_quit_t on_quit
) {
	return false;
}
