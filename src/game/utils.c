// Copyright (c) Kuba Szczodrzyński 2024-12-28.

#include "include.h"

bool game_add_endpoint(game_t *game, net_endpoint_t *endpoint) {
	net_endpoint_t *item = net_endpoint_dup(endpoint);
	if (item == NULL)
		return false;
	SDL_WITH_MUTEX(game->mutex) {
		DL_APPEND(game->endpoints, item);
	}
	return true;
}
