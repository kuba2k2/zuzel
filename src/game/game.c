// Copyright (c) Kuba Szczodrzyński 2024-12-27.

#include "include.h"

static int game_thread(game_t *game);
static net_err_t game_select_cb(net_endpoint_t *endpoint, game_t *game);
static net_err_t game_respond(net_endpoint_t *endpoint, pkt_t *recv_pkt);

game_t *game_init() {
	game_t *game;
	MALLOC(game, sizeof(*game), goto cleanup);

	if ((game->mutex = SDL_CreateMutex()) == NULL)
		SDL_ERROR("SDL_CreateMutex()", goto cleanup);

	// create a pipe for incoming packets
	{
		net_endpoint_t *pipe;
		MALLOC(pipe, sizeof(*pipe), goto cleanup);
		DL_APPEND(game->endpoints, pipe);
		if (pipe(pipe->pipe) != 0)
			LT_ERR(F, goto cleanup, "Couldn't create a pipe");
#if WIN32
		pipe->event = WSACreateEvent();
#endif
	}

	// generate a game name
	snprintf(game->name, sizeof(game->name) - 1, "%s's Game", SETTINGS->player_name);
	// generate a game key
	char *ch = game->key;
	for (int i = 0; i < sizeof(game->key) - 1; i++) {
		int num = '0' + rand() % 36;
		if (num > '9')
			num += 'A' - '9' - 1;
		*ch++ = num;
	}
	// set some other default settings
	game->is_public = false;
	game->speed		= 3;
	game->state		= GAME_IDLE;

	// finally, start the game thread
	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)game_thread, "game", game);
	SDL_DetachThread(thread);
	if (thread == NULL)
		SDL_ERROR("SDL_CreateThread()", goto cleanup);

	return game;

cleanup:
	game_free(game);
	return NULL;
}

void game_free(game_t *game) {
	if (game == NULL)
		return;
	SDL_DestroyMutex(game->mutex);
	free(game);
}

static int game_thread(game_t *game) {
	if (game == NULL)
		return -1;

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
	return game_respond(endpoint, &endpoint->recv.pkt);

closed:
	SDL_WITH_MUTEX(game->mutex) {
		net_endpoint_t *item, *tmp;
		DL_FOREACH_SAFE(game->endpoints, item, tmp) {
			if (item != endpoint)
				continue;
			DL_DELETE(game->endpoints, item);
			net_endpoint_close(endpoint);
			free(endpoint);
		}
	}
	return NET_ERR_OK;
}

static net_err_t game_respond(net_endpoint_t *endpoint, pkt_t *recv_pkt) {
	switch (recv_pkt->hdr.type) {
		case PKT_PING: {
			pkt_ping_t pkt = {
				.hdr.type	 = PKT_PING,
				.seq		 = recv_pkt->ping.seq,
				.is_response = true,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}

		default: {
			pkt_error_t pkt = {
				.hdr.type = PKT_ERROR,
				.error	  = GAME_ERR_INVALID_STATE,
			};
			return net_pkt_send(endpoint, (pkt_t *)&pkt);
		}
	}
}
