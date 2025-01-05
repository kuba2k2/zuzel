// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "game.h"

static int game_thread(game_t *game);
static net_err_t game_select_read_cb(net_endpoint_t *endpoint, game_t *game);
static void game_select_err_cb(net_endpoint_t *endpoint, game_t *game, net_err_t err);

static game_t *game_list		  = NULL;
static SDL_mutex *game_list_mutex = NULL;

game_t *game_init(pkt_game_data_t *pkt_data) {
	game_t *game;
	MALLOC(game, sizeof(*game), goto cleanup);

	// create a pipe for incoming packets
	{
		net_endpoint_t pipe = {0};
		net_endpoint_pipe(&pipe);
		pipe.pipe.broadcast_sdl = true;
		game_add_endpoint(game, &pipe);
	}

	SDL_WITH_MUTEX(game->mutex) {
		if (pkt_data == NULL)
			game_set_data_default(game);
		else
			game_process_packet(game, (pkt_t *)pkt_data, NULL);
		game->is_client = pkt_data != NULL;
	}

	// finally, start the game thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)game_thread, "game", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", goto cleanup);

	if (!game->is_client) {
		// only use game_list server-side
		SDL_WITH_MUTEX(game_list_mutex) {
			DL_APPEND(game_list, game);
		}
	}

	return game;

cleanup:
	game_free(game);
	return NULL;
}

void game_free(game_t *game) {
	if (game == NULL)
		return;
	// remove the game from the global list
	SDL_WITH_MUTEX(game_list_mutex) {
		DL_DELETE(game_list, game);
	}
	// close and free all endpoints
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *endpoint, *tmp;
		DL_FOREACH_SAFE(game->endpoints, endpoint, tmp) {
			DL_DELETE(game->endpoints, endpoint);
			net_endpoint_free(endpoint);
			SDL_DestroyMutex(endpoint->mutex);
			free(endpoint);
		}
	}
	// free remaining members
	SDL_DestroyMutex(game->mutex);
	free(game);
}

game_t *game_get_by_key(const char *key) {
	game_t *game;
	int count = 0;
	SDL_WITH_MUTEX(game_list_mutex) {
		DL_FOREACH(game_list, game) {
			count++;
			if (strnicmp(game->key, key, GAME_KEY_LEN) == 0)
				break;
		}
	}
	if (game == NULL && count == 1 && key[0] == '\0' && game_list->is_public)
		return game_list;
	return game;
}

game_t *game_get_list(SDL_mutex **mutex) {
	if (mutex != NULL)
		*mutex = game_list_mutex;
	return game_list;
}

static int game_thread(game_t *game) {
	if (game == NULL)
		return -1;
	char thread_name[19];
	snprintf(thread_name, sizeof(thread_name), "game-%s-%s", game->is_client ? "client" : "server", game->key);
	lt_log_set_thread_name(thread_name);

	LT_I("Game: starting '%s' (key: %s)", game->name, game->key);

	while (!game->stop) {
		// wait for incoming data
		net_err_t err = net_endpoint_select(
			game->endpoints,
			game->mutex,
			(net_select_read_cb_t)game_select_read_cb,
			(net_select_err_cb_t)game_select_err_cb,
			game
		);
		if (err != NET_ERR_OK)
			goto cleanup;
		SDL_Delay(100);
	}

cleanup:
	LT_I("Game: stopping '%s' (key: %s)", game->name, game->key);
	game_free(game);
	return 0;
}

static net_err_t game_select_read_cb(net_endpoint_t *endpoint, game_t *game) {
	net_err_t ret = net_pkt_recv(endpoint);
	if (ret < NET_ERR_OK)
		return ret;
	if (ret != NET_ERR_OK_PACKET)
		// continue if packet is not fully received yet
		return NET_ERR_OK;

	// valid packet received, process it and send a response
	if (game_process_packet(game, &endpoint->recv.pkt, endpoint) == false)
		// if 'false', packet was consumed by processing
		return NET_ERR_OK;

	// otherwise, broadcast the packet to other endpoints
	SDL_WITH_MUTEX(game->mutex) {
		net_pkt_broadcast(game->endpoints, &endpoint->recv.pkt, endpoint);
	}
	return NET_ERR_OK;
}

static void game_select_err_cb(net_endpoint_t *endpoint, game_t *game, net_err_t err) {
	if (err == NET_ERR_CLIENT_CLOSED)
		LT_I("Game: connection closed from %s", net_endpoint_str(endpoint));
	else
		LT_E("Game: connection error from %s", net_endpoint_str(endpoint));
	game_del_endpoint(game, endpoint);
}
