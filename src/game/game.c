// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

static int game_thread(game_t *game);
static net_err_t game_select_cb(net_endpoint_t *endpoint, game_t *game);
static net_err_t game_respond(net_endpoint_t *endpoint, game_t *game, pkt_t *recv_pkt);

static game_t *game_list		  = NULL;
static SDL_mutex *game_list_mutex = NULL;

game_t *game_init() {
	game_t *game;
	MALLOC(game, sizeof(*game), goto cleanup);

	if ((game->mutex = SDL_CreateMutex()) == NULL)
		SDL_ERROR("SDL_CreateMutex()", goto cleanup);

	// create a pipe for incoming packets
	{
		net_endpoint_t pipe = {0};
		net_endpoint_pipe(&pipe);
		game_add_endpoint(game, &pipe);
	}

	// generate a game name
	snprintf(game->name, sizeof(game->name) - 1, "%s's Game", SETTINGS->player_name);
	// generate a game key
	do {
		char *ch = game->key;
		for (int i = 0; i < sizeof(game->key) - 1; i++) {
			int num = '0' + rand() % 36;
			if (num > '9')
				num += 'A' - '9' - 1;
			*ch++ = num;
		}
	} while (game_get_by_key(game->key) != NULL);
	// set some other default settings
	game->is_public = false;
	game->speed		= 3;
	game->state		= GAME_IDLE;

	// finally, start the game thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)game_thread, "game", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", goto cleanup);

	if (game_list_mutex == NULL)
		game_list_mutex = SDL_CreateMutex();
	SDL_WITH_MUTEX(game_list_mutex) {
		DL_APPEND(game_list, game);
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
			free(endpoint);
		}
	}
	// free remaining members
	SDL_DestroyMutex(game->mutex);
	free(game);
}

game_t *game_get_by_key(const char *key) {
	game_t *game;
	SDL_WITH_MUTEX(game_list_mutex) {
		DL_FOREACH(game_list, game) {
			if (strnicmp(game->key, key, GAME_KEY_LEN) == 0)
				break;
		}
	}
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
	lt_log_set_thread_name("game");

	LT_I("Game: starting '%s' (key: %s)", game->name, game->key);

	while (1) {
		// wait for incoming data
		if (net_endpoint_select(game->endpoints, game->mutex, (net_select_cb_t)game_select_cb, game) != NET_ERR_OK)
			goto cleanup;
	}

cleanup:
	return 0;
}

static net_err_t game_select_cb(net_endpoint_t *endpoint, game_t *game) {
	net_err_t ret = net_pkt_recv(endpoint);
	if (ret == NET_ERR_RECV)
		return ret;
	if (ret == NET_ERR_CLIENT_CLOSED) {
		LT_I("Game: connection closed from %s:%d", inet_ntoa(endpoint->addr.sin_addr), ntohs(endpoint->addr.sin_port));
		goto closed;
	}
	if (ret != NET_ERR_OK_PACKET)
		// continue if packet is not fully received yet
		return NET_ERR_OK;

	// valid packet received, process it and send a response
	return game_respond(endpoint, game, &endpoint->recv.pkt);

closed:
	game_del_endpoint(game, endpoint);
	return NET_ERR_OK;
}

static net_err_t game_respond(net_endpoint_t *endpoint, game_t *game, pkt_t *recv_pkt) {
	switch (recv_pkt->hdr.type) {
		case PKT_PING: {
			pkt_ping_t pkt = {
				.hdr.type	 = PKT_PING,
				.seq		 = recv_pkt->ping.seq,
				.is_response = true,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}

		case PKT_ERROR:
			return NET_ERR_OK;

		case PKT_SEND_UPDATE:
			game_send_update(game, endpoint, NULL);
			return NET_ERR_OK;

		default: {
			pkt_error_t pkt = {
				.hdr.type = PKT_ERROR,
				.error	  = GAME_ERR_INVALID_STATE,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}
	}
}
