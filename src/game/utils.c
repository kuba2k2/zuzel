// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "game.h"

void game_set_data_default(game_t *game) {
	// generate a game name
	if (SETTINGS->game_name != NULL)
		strncpy(game->name, SETTINGS->game_name, sizeof(game->name) - 1);
	else
		snprintf(game->name, sizeof(game->name), "%s's Game", SETTINGS->player_name);

	// generate a game key
	do {
		char *ch = game->key;
		for (int i = 0; i < sizeof(game->key) - 1; i++) {
			int num = '0' + rand() % 36;
			if (num > '9')
				num += 'A' - '9' - 1;
			*ch++ = (char)num;
		}
	} while (game_get_by_key(game->key) != NULL);

	// set some other default settings
	game->is_public = false;
	game->speed		= 3;
	game->state		= GAME_IDLE;
}

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
	}
}
