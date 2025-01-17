// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

void game_print_error(game_err_t error) {
	switch (error) {
		case GAME_ERR_OK:
			LT_E("Received error packet with unspecified reason");
			break;
		case GAME_ERR_INVALID_STATE:
			LT_E("Operation invalid in the current game state");
			break;
		case GAME_ERR_NOT_FOUND:
			LT_E("Game not found by the specified key");
			break;
		case GAME_ERR_NO_PLAYER:
			LT_E("Player not found by the specified ID");
			break;
		case GAME_ERR_SERVER_ERROR:
			LT_E("Internal server error");
			break;
	}
}

bool game_send_error(net_endpoint_t *endpoint, game_err_t error) {
	pkt_error_t pkt = {
		.hdr.type = PKT_ERROR,
		.error	  = error,
	};
	net_pkt_send(endpoint, (pkt_t *)&pkt);
	return false;
}

void game_stop_all() {
	SDL_mutex *game_list_mutex;
	game_t *game_list = game_get_list(&game_list_mutex);
	if (game_list == NULL)
		return;
	SDL_WITH_MUTEX(game_list_mutex) {
		game_t *game, *tmp;
		DL_FOREACH_SAFE(game_list, game, tmp) {
			game_stop(game);
		}
	}
}

game_t *game_get_by_key(char *key) {
	SDL_mutex *game_list_mutex;
	game_t *game_list = game_get_list(&game_list_mutex);
	if (game_list == NULL)
		return NULL;

	// make it uppercase
	char *ch = key;
	while (*ch != '\0') {
		*ch = (char)toupper(*ch);
		ch++;
	}

	game_t *game = NULL;
	int count	 = 0;
	SDL_WITH_MUTEX(game_list_mutex) {
		DL_FOREACH(game_list, game) {
			count++;
			if (strncmp(game->key, key, GAME_KEY_LEN) == 0)
				break;
		}
	}
	if (game == NULL && count == 1 && key[0] == '\0' && game_list->is_public)
		return game_list;
	return game;
}
